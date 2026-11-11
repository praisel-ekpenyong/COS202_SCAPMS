#include "controller/PatientSorter.h"

#include "controller/SearchEngine.h"
#include "model/Appointment.h"
#include "model/AppointmentStatus.h"

#include <memory>
#include <unordered_map>

namespace scapms::controller {

namespace {

using Record = const model::PatientRecord*;

// Decorate–sort–undecorate: each record is paired with its key computed
// ONCE per sort, so comparisons never re-derive anything (case-folding and
// ID parsing would otherwise run inside every one of the O(n²)/O(n log n)
// comparisons).
template <typename Key>
struct Keyed {
    Key key;
    Record record;
};

// The ordering definitions (caseFoldedName, patientIdNumber) are shared with
// SearchEngine - see SearchEngine.h. PatientSorter computes its keys with
// them, and the engine folds/parses its probes with them: one definition each,
// so key order and probe order remain consistent.

// One pass over ALL appointments (O(A)) instead of appointmentsFor per
// patient (O(n·A)) - the same const database read API, scanned once, and
// shared by every patient in the sort.
// A patient's next upcoming appointment = their earliest PENDING appointment
// dated today or later ("YYYY-MM-DD HH:MM"; ISO fields are fixed-width, so
// lexicographic order is chronological - same trick as isOverdue). Cancelled
// appointments are not happening, completed ones already happened, and an
// overdue pending appointment is past, not upcoming: none of them count.
std::unordered_map<std::string, std::string> nextAppointmentStarts(
    const model::ClinicDatabase& database, const std::string& today) {
    std::unordered_map<std::string, std::string> next;  // patientId -> "date time"
    for (const std::unique_ptr<model::Appointment>& appointment : database.appointments()) {
        if (appointment->status() != model::AppointmentStatus::Pending) continue;
        if (appointment->date() < today) continue;
        std::string start = appointment->date() + " " + appointment->time();
        std::string& kept = next[appointment->patientId()];
        if (kept.empty() || start < kept) kept = std::move(start);
    }
    return next;
}

// The appointment-date key: records with an upcoming appointment sort by its
// start; records without one share the "no key" group, which sorts LAST.
// Both algorithm variants keep that group contiguous at the end.
struct UpcomingKey {
    bool has = false;
    std::string start;  // "date time"; meaningful only when has
};

bool operator<(const UpcomingKey& a, const UpcomingKey& b) {
    if (a.has != b.has) return a.has;          // has-appointment sorts first
    return a.has && a.start < b.start;         // both without one → equal
}

template <typename Key>
bool lessKey(const Keyed<Key>& a, const Keyed<Key>& b) {
    return a.key < b.key;
}

// Runs the chosen sorting algorithm over the decorated vector.
template <typename Key>
void runSort(std::vector<Keyed<Key>>& keyed, SortAlgorithm algorithm) {
    if (algorithm == SortAlgorithm::SelectionSort)
        selectionSort(keyed, lessKey<Key>);
    else
        mergeSort(keyed, lessKey<Key>);
}

}  // namespace

std::string sortKeyName(SortKey key) {
    switch (key) {
        case SortKey::Name: return "Name";
        case SortKey::Id: return "ID";
        case SortKey::Age: return "Age";
        case SortKey::AppointmentDate: return "Appointment date";
    }
    return "";
}

std::string sortAlgorithmName(SortAlgorithm algorithm) {
    switch (algorithm) {
        case SortAlgorithm::SelectionSort: return "Selection Sort";
        case SortAlgorithm::MergeSort: return "Merge Sort";
    }
    return "";
}

std::string sortAlgorithmDescription(SortAlgorithm algorithm) {
    switch (algorithm) {
        case SortAlgorithm::SelectionSort:
            return "Selection Sort: Simple in-place O(n²) algorithm with zero extra memory allocation; optimal for small record lists.";
        case SortAlgorithm::MergeSort:
            return "Merge Sort: Predictable O(n log n) divide-and-conquer algorithm with stable ordering; optimal for larger datasets and report generation.";
    }
    return "";
}

SortResult PatientSorter::sort(const ClinicManager& manager, SortKey key,
                               SortAlgorithm algorithm, const std::string& today) {
    const std::string day = today.empty() ? ClinicManager::currentDate() : today;
    const model::ClinicDatabase& database = manager.database();

    SortResult result;
    result.key = key;
    result.algorithm = algorithm;
    result.revisionAtSort = manager.revision();  // handshake stamp
    result.patients.reserve(database.patients().size());

    switch (key) {
        case SortKey::Name: {
            std::vector<Keyed<std::string>> keyed;
            keyed.reserve(database.patients().size());
            for (const std::unique_ptr<model::PatientRecord>& record : database.patients())
                keyed.push_back(Keyed<std::string>{caseFoldedName(record->name()), record.get()});
            runSort(keyed, algorithm);
            for (const Keyed<std::string>& entry : keyed) result.patients.push_back(entry.record);
            break;
        }
        case SortKey::Id: {
            std::vector<Keyed<unsigned>> keyed;
            keyed.reserve(database.patients().size());
            for (const std::unique_ptr<model::PatientRecord>& record : database.patients())
                keyed.push_back(Keyed<unsigned>{patientIdNumber(record->id()), record.get()});
            runSort(keyed, algorithm);
            for (const Keyed<unsigned>& entry : keyed) result.patients.push_back(entry.record);
            break;
        }
        case SortKey::Age: {
            std::vector<Keyed<int>> keyed;
            keyed.reserve(database.patients().size());
            for (const std::unique_ptr<model::PatientRecord>& record : database.patients())
                keyed.push_back(Keyed<int>{record->age(), record.get()});
            runSort(keyed, algorithm);
            for (const Keyed<int>& entry : keyed) result.patients.push_back(entry.record);
            break;
        }
        case SortKey::AppointmentDate: {
            const std::unordered_map<std::string, std::string> next =
                nextAppointmentStarts(database, day);
            std::vector<Keyed<UpcomingKey>> keyed;
            keyed.reserve(database.patients().size());
            for (const std::unique_ptr<model::PatientRecord>& record : database.patients()) {
                const auto found = next.find(record->id());
                if (found == next.end())
                    keyed.push_back(Keyed<UpcomingKey>{UpcomingKey{}, record.get()});
                else
                    keyed.push_back(Keyed<UpcomingKey>{UpcomingKey{true, found->second}, record.get()});
            }
            runSort(keyed, algorithm);
            for (const Keyed<UpcomingKey>& entry : keyed) result.patients.push_back(entry.record);
            break;
        }
    }

    return result;
}

bool installSearchView(SearchEngine& engine, const SortResult& result) {
    switch (result.key) {
        case SortKey::Name:
            // The name key is case-insensitive - the engine must probe
            // the copy in that same order (SearchOrder::CaseFolded).
            return engine.setSortedView(SearchField::Name, result.patients,
                                        result.revisionAtSort,
                                        SearchOrder::CaseFolded);
        case SortKey::Id:
            // The ID key is the numeric part - 5-digit IDs order differently
            // byte-wise, so the probe must be numeric too.
            return engine.setSortedView(SearchField::Id, result.patients,
                                        result.revisionAtSort,
                                        SearchOrder::Numeric);
        case SortKey::Age:
        case SortKey::AppointmentDate:
            // No SearchField exists for these keys: the engine searches IDs
            // and names only. Refuse instead of silently keeping an old view.
            return false;
    }
    return false;
}

}  // namespace scapms::controller
