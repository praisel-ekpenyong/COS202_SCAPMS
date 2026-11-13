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
    Name,             // case-insensitive A-Z
    Id,               // numeric part of "P-xxxx"
    Age,              // ascending
    AppointmentDate,  // sorts by next appointment (patients without one go last)
};

// Supported sorting algorithms.
enum class SortAlgorithm {
    SelectionSort,
    MergeSort,
};

// Result of a sort operation.
struct SortResult {
    std::vector<const model::PatientRecord*> patients;
    SortKey key = SortKey::Name;
    SortAlgorithm algorithm = SortAlgorithm::MergeSort;
    unsigned revisionAtSort = 0;
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

// Passes sorted results to search engine if compatible with binary search.
bool installSearchView(SearchEngine& engine, const SortResult& result);

}  // namespace scapms::controller
