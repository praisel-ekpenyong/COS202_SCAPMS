#pragma once
// Sorts patient records by Name, ID, Age, or Appointment Date
// using custom Selection Sort or Merge Sort implementations.
#include "controller/ClinicManager.h"
#include "controller/SortAlgorithms.h"
#include "model/PatientRecord.h"

#include <string>
#include <vector>

namespace scapms::controller {

class SearchEngine;

// Sort keys selectable in GUI dropdown.
enum class SortKey {
    Name,             // case-insensitive A–Z
    Id,               // numeric part of "P-xxxx"
    Age,              // ascending
    AppointmentDate,  // next upcoming appointment; without one → last
};

// Supported sorting algorithms.
enum class SortAlgorithm {
    SelectionSort,  // O(n²) comparisons, in-place, NOT stable (documented)
    MergeSort,      // O(n log n), stable - equal keys keep their order
};

// Stores the sorted pointer vector along with sort metadata.
struct SortResult {
    std::vector<const model::PatientRecord*> patients;  // sorted working copy
    SortKey key = SortKey::Name;
    SortAlgorithm algorithm = SortAlgorithm::MergeSort;
    unsigned revisionAtSort = 0;  // manager.revision when this copy was made
};

// Display label and description helpers for UI presentation.
std::string sortKeyName(SortKey key);
std::string sortAlgorithmName(SortAlgorithm algorithm);
std::string sortAlgorithmDescription(SortAlgorithm algorithm);

class PatientSorter {
public:
    // Returns a sorted vector of patient pointers without altering database order.
    static SortResult sort(const ClinicManager& manager, SortKey key,
                           SortAlgorithm algorithm, const std::string& today = "");
};

// Updates search engine with latest sorted view if key is searchable and revision is current.
bool installSearchView(SearchEngine& engine, const SortResult& result);

}  // namespace scapms::controller
