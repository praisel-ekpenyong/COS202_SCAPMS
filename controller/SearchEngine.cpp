#include "controller/SearchEngine.h"

#include "controller/ClinicManager.h"
#include "model/ClinicDatabase.h"
#include "model/PatientRecord.h"

#include <cctype>
#include <cstddef>

namespace scapms::controller {
namespace {
constexpr std::size_t kNotFound = static_cast<std::size_t>(-1);
}  // namespace

std::string techniqueLabel(SearchTechnique technique) {
    switch (technique) {
        case SearchTechnique::Linear:
            return "Linear search";
        case SearchTechnique::Binary:
            return "Binary search";
        case SearchTechnique::Recursive:
            return "Recursive search";
    }
    return "Linear search";
}

std::string caseFoldedName(const std::string& name) {
    std::string folded;
    folded.reserve(name.size());
    for (char c : name)
        folded.push_back(
            static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    return folded;
}

unsigned patientIdNumber(const std::string& id) {
    const std::size_t digits = id.find('-') + 1;
    unsigned value = 0;
    for (std::size_t i = digits; i < id.size() && id[i] >= '0' && id[i] <= '9'; ++i)
        value = value * 10 + static_cast<unsigned>(id[i] - '0');
    return value;
}

bool SearchResult::found() const {
    return !matches.empty();
}

const model::PatientRecord* SearchResult::first() const {
    return matches.empty() ? nullptr : matches.front();
}

SearchEngine::SearchEngine(const ClinicManager& manager) : manager_(manager) {}

// --- sorted view (sorting handshake) -------------------------------------

bool SearchEngine::setSortedView(
    SearchField field,
    std::vector<const model::PatientRecord*> sortedRecords,
    unsigned revision, SearchOrder order) {
    if (revision != manager_.revision()) {
        return false;
    }
    if (field == SearchField::Id && order == SearchOrder::CaseFolded) {
        return false;
    }
    if (field == SearchField::Name && order == SearchOrder::Numeric) {
        return false;
    }
    std::vector<const model::PatientRecord*> cleaned;
    cleaned.reserve(sortedRecords.size());
    for (const model::PatientRecord* record : sortedRecords) {
        if (record != nullptr) {
            cleaned.push_back(record);
        }
    }
    sortedView_.installed = true;
    sortedView_.field = field;
    sortedView_.order = order;
    sortedView_.revision = revision;
    sortedView_.records = std::move(cleaned);
    return true;
}

void SearchEngine::clearSortedView() {
    sortedView_ = SortedView{};
}

bool SearchEngine::hasSortedView() const {
    return sortedView_.installed;
}

SearchField SearchEngine::sortedViewField() const {
    return sortedView_.field;
}

unsigned SearchEngine::sortedViewRevision() const {
    return sortedView_.revision;
}

SearchOrder SearchEngine::sortedViewOrder() const {
    return sortedView_.order;
}

bool SearchEngine::freshSortedViewFor(SearchField field) const {
    return sortedView_.installed && sortedView_.field == field &&
           sortedView_.revision == manager_.revision() &&
           sortedView_.records.size() ==
               manager_.database().patients().size();
}

std::size_t SearchEngine::binarySearchById(
    const std::vector<const model::PatientRecord*>& records,
    const std::string& id, SearchOrder order) {
    std::size_t lo = 0;
    std::size_t hi = records.size();
    if (order == SearchOrder::Numeric) {
        const unsigned key = patientIdNumber(id);
        while (lo < hi) {
            const std::size_t mid = lo + (hi - lo) / 2;
            if (patientIdNumber(records[mid]->id()) < key) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        while (lo < records.size() && patientIdNumber(records[lo]->id()) == key) {
            if (records[lo]->id() == id) {
                return lo;
            }
            ++lo;
        }
        return kNotFound;
    }
    while (lo < hi) {
        const std::size_t mid = lo + (hi - lo) / 2;
        if (records[mid]->id() < id) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    if (lo < records.size() && records[lo]->id() == id) {
        return lo;
    }
    return kNotFound;
}

std::size_t SearchEngine::binarySearchByName(
    const std::vector<const model::PatientRecord*>& records,
    const std::string& name, NameMatch mode, SearchOrder order) {
    std::size_t lo = 0;
    std::size_t hi = records.size();
    if (order == SearchOrder::CaseFolded) {
        const std::string foldedQuery = caseFoldedName(name);
        while (lo < hi) {
            const std::size_t mid = lo + (hi - lo) / 2;
            if (caseFoldedName(records[mid]->name()) < foldedQuery) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        if (lo >= records.size()) {
            return kNotFound;
        }
        if (mode == NameMatch::Prefix) {
            return nameMatchesQuery(records[lo]->name(), name, mode, order)
                       ? lo
                       : kNotFound;
        }
        while (lo < records.size() &&
               caseFoldedName(records[lo]->name()) == foldedQuery) {
            if (records[lo]->name() == name) {
                return lo;
            }
            ++lo;
        }
        return kNotFound;
    }
    while (lo < hi) {
        const std::size_t mid = lo + (hi - lo) / 2;
        if (records[mid]->name() < name) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    if (lo < records.size() &&
        nameMatchesQuery(records[lo]->name(), name, mode, order)) {
        return lo;
    }
    return kNotFound;
}

bool SearchEngine::nameMatchesQuery(const std::string& candidate,
                                    const std::string& name, NameMatch mode,
                                    SearchOrder order) {
    if (order == SearchOrder::CaseFolded) {
        const std::string foldedCandidate = caseFoldedName(candidate);
        const std::string foldedName = caseFoldedName(name);
        if (mode == NameMatch::Exact) {
            return foldedCandidate == foldedName;
        }
        return foldedCandidate.size() >= foldedName.size() &&
               foldedCandidate.compare(0, foldedName.size(), foldedName) == 0;
    }
    if (mode == NameMatch::Exact) {
        return candidate == name;
    }
    return candidate.size() >= name.size() &&
           candidate.compare(0, name.size(), name) == 0;
}

SearchResult SearchEngine::findById(const std::string& id) const {
    if (freshSortedViewFor(SearchField::Id)) {
        SearchResult sorted;
        sorted.technique = SearchTechnique::Binary;
        const std::size_t index = binarySearchById(
            sortedView_.records, id, sortedView_.order);
        if (index != kNotFound) {
            sorted.matches.push_back(sortedView_.records[index]);
        }
        return sorted;
    }

    SearchResult result;
    result.technique = SearchTechnique::Linear;
    for (const auto& patient : manager_.database().patients()) {
        if (patient->id() == id) {
            result.matches.push_back(patient.get());
            break;
        }
    }
    return result;
}

SearchResult SearchEngine::findByName(const std::string& name,
                                      NameMatch mode) const {
    if (freshSortedViewFor(SearchField::Name)) {
        SearchResult sorted;
        sorted.technique = SearchTechnique::Binary;
        const std::size_t index = binarySearchByName(
            sortedView_.records, name, mode, sortedView_.order);
        if (index != kNotFound) {
            sorted.matches.push_back(sortedView_.records[index]);
        }
        return sorted;
    }

    SearchResult result;
    result.technique = SearchTechnique::Linear;
    for (const auto& patient : manager_.database().patients()) {
        if (nameMatchesQuery(patient->name(), name, mode,
                             SearchOrder::ByteWise)) {
            result.matches.push_back(patient.get());
            break;
        }
    }
    return result;
}

SearchResult SearchEngine::findByNameRecursive(
    const std::string& name, NameMatch mode) const {
    std::vector<const model::PatientRecord*> live;
    for (const auto& patient : manager_.database().patients()) {
        live.push_back(patient.get());
    }

    SearchResult result;
    result.technique = SearchTechnique::Recursive;
    if (const model::PatientRecord* hit = recursiveNameScan(live, 0, name, mode)) {
        result.matches.push_back(hit);
    }
    return result;
}

const model::PatientRecord* SearchEngine::recursiveNameScan(
    const std::vector<const model::PatientRecord*>& records,
    std::size_t index, const std::string& name, NameMatch mode) const {
    if (index >= records.size()) {
        return nullptr;
    }
    const model::PatientRecord* record = records[index];
    if (nameMatchesQuery(record->name(), name, mode, SearchOrder::ByteWise)) {
        return record;
    }
    return recursiveNameScan(records, index + 1, name, mode);
}

SearchResult SearchEngine::filterByCategory(
    model::PatientCategory category) const {
    SearchResult result;
    result.technique = SearchTechnique::Linear;
    for (const auto& patient : manager_.database().patients()) {
        if (patient->category() == category) {
            result.matches.push_back(patient.get());
        }
    }
    return result;
}

}  // namespace scapms::controller
