#pragma once
// Search engine supporting linear, binary, and recursive search.
#include "model/PatientCategory.h"

#include <cstddef>
#include <string>
#include <vector>

namespace scapms::model {
class PatientRecord;
}

namespace scapms::controller {

class ClinicManager;

// Field to search by
enum class SearchField { Id, Name };

// Search algorithms
enum class SearchTechnique { Linear, Binary, Recursive };

// Name matching mode
enum class NameMatch { Exact, Prefix };

// Ordering used for sorting
enum class SearchOrder { ByteWise, CaseFolded, Numeric };

// Convert string to lowercase
std::string caseFoldedName(const std::string& name);
// Extracts number from patient ID (e.g. 1 from P-0001)
unsigned patientIdNumber(const std::string& id);

// Returns algorithm name for display
std::string techniqueLabel(SearchTechnique technique);

// Search result
struct SearchResult {
    std::vector<const model::PatientRecord*> matches;
    SearchTechnique technique = SearchTechnique::Linear;

    bool found() const;
    const model::PatientRecord* first() const;
};

class SearchEngine {
public:
    explicit SearchEngine(const ClinicManager& manager);

    // Searches

    // Searches patient by ID using binary search if sorted, linear otherwise.
    SearchResult findById(const std::string& id) const;

    // Searches patient by name (exact or prefix).
    SearchResult findByName(const std::string& name,
                            NameMatch mode = NameMatch::Exact) const;

    // Recursive search across patient records by name.
    SearchResult findByNameRecursive(const std::string& name,
                                     NameMatch mode = NameMatch::Exact) const;

    // Filters patients by category using sequential scan.
    SearchResult filterByCategory(model::PatientCategory category) const;

    // Sorted view for binary search

    // Sets a sorted view for binary search.
    bool setSortedView(SearchField field,
                       std::vector<const model::PatientRecord*> sortedRecords,
                       unsigned revision,
                       SearchOrder order = SearchOrder::ByteWise);

    // Clears the active sorted view.
    void clearSortedView();

    bool hasSortedView() const;
    SearchField sortedViewField() const;
    unsigned sortedViewRevision() const;
    SearchOrder sortedViewOrder() const;

private:
    // Returns true if the sorted view is up-to-date for the given field.
    bool freshSortedViewFor(SearchField field) const;

    // Custom binary search implementation returning index or not found.
    static std::size_t binarySearchById(
        const std::vector<const model::PatientRecord*>& records,
        const std::string& id, SearchOrder order);
    static std::size_t binarySearchByName(
        const std::vector<const model::PatientRecord*>& records,
        const std::string& name, NameMatch mode, SearchOrder order);

    // Helper comparing name to query according to match mode.
    static bool nameMatchesQuery(const std::string& candidate,
                                 const std::string& name, NameMatch mode,
                                 SearchOrder order);

    // State of installed sorted view.
    struct SortedView {
        bool installed = false;
        SearchField field = SearchField::Id;
        SearchOrder order = SearchOrder::ByteWise;
        unsigned revision = 0;
        std::vector<const model::PatientRecord*> records;
    };

    // Recursive search helper step.
    const model::PatientRecord* recursiveNameScan(
        const std::vector<const model::PatientRecord*>& records,
        std::size_t index, const std::string& name, NameMatch mode) const;

    const ClinicManager& manager_;
    SortedView sortedView_;
};

}  // namespace scapms::controller
