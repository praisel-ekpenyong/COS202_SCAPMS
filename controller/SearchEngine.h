#pragma once
// SearchEngine implements Linear, Binary, and Recursive search algorithms
// over patient records without using standard library search algorithms.
#include "model/PatientCategory.h"

#include <cstddef>
#include <string>
#include <vector>

namespace scapms::model {
class PatientRecord;
}

namespace scapms::controller {

class ClinicManager;

// Which record field a search (or a sorted view) is keyed on.
enum class SearchField { Id, Name };

// Searching algorithms implemented.
enum class SearchTechnique { Linear, Binary, Recursive };

// String matching modes for name queries.
enum class NameMatch { Exact, Prefix };

// Ordering scheme used in sorted views.
enum class SearchOrder { ByteWise, CaseFolded, Numeric };

// Helper functions for case-folding and ID parsing.
// caseFoldedName: lower-cased ASCII letters (names are letters and spaces).
std::string caseFoldedName(const std::string& name);
// patientIdNumber: the digits after the dash of "P-0001", read as a number.
unsigned patientIdNumber(const std::string& id);

// Text label for status bar display.
std::string techniqueLabel(SearchTechnique technique);

// Result of a search query, containing matched pointers and technique used.
struct SearchResult {
    std::vector<const model::PatientRecord*> matches;
    SearchTechnique technique = SearchTechnique::Linear;

    // true when at least one record matched.
    bool found() const;
    // The first match, or nullptr when nothing matched - the find-by-x
    // searches produce at most one match, filters produce many.
    const model::PatientRecord* first() const;
};

// Stateless search behaviour over one manager's records. The manager must
// outlive the engine (the engine stores only a reference).
class SearchEngine {
public:
    explicit SearchEngine(const ClinicManager& manager);

    // --- searches (each reports its technique in the result) -------------

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

    // --- sorted view (sorting handshake) ----------------------------------

    // Installs a sorted view for binary search; falls back to linear if stale.
    bool setSortedView(SearchField field,
                       std::vector<const model::PatientRecord*> sortedRecords,
                       unsigned revision,
                       SearchOrder order = SearchOrder::ByteWise);

    // Clears the active sorted view.
    void clearSortedView();

    bool hasSortedView() const;          // a view is installed (maybe stale)
    SearchField sortedViewField() const; // meaningful only when hasSortedView
    unsigned sortedViewRevision() const; // meaningful only when hasSortedView
    SearchOrder sortedViewOrder() const; // meaningful only when hasSortedView

private:
    // Binary fast path available: a view is installed, covers the right
    // field, carries the current revision, and holds every live record.
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
