#pragma once
// Custom implementations of Selection Sort and Merge Sort.
#include <cstddef>
#include <utility>
#include <vector>

namespace scapms::controller {

// Selection sort: finds minimum element and swaps into place.
template <typename T, typename LessThan>
void selectionSort(std::vector<T>& items, LessThan lessThan) {
    const std::size_t count = items.size();
    if (count < 2) return;
    for (std::size_t slot = 0; slot + 1 < count; ++slot) {
        std::size_t smallest = slot;
        for (std::size_t probe = slot + 1; probe < count; ++probe)
            if (lessThan(items[probe], items[smallest])) smallest = probe;
        if (smallest != slot) std::swap(items[slot], items[smallest]);
    }
}

namespace detail {

// Merges sorted sub-arrays items[lo, mid) and items[mid, hi] using scratch buffer.
template <typename T, typename LessThan>
void mergeRuns(std::vector<T>& items, std::vector<T>& scratch, std::size_t lo,
               std::size_t mid, std::size_t hi, LessThan lessThan) {
    std::size_t left = lo, right = mid, out = lo;
    while (left < mid && right < hi) {
        if (lessThan(items[right], items[left]))
            scratch[out++] = items[right++];
        else
            scratch[out++] = items[left++];
    }
    while (left < mid) scratch[out++] = items[left++];
    while (right < hi) scratch[out++] = items[right++];
    for (std::size_t i = lo; i < hi; ++i) items[i] = scratch[i];
}

template <typename T, typename LessThan>
void mergeSortRange(std::vector<T>& items, std::vector<T>& scratch, std::size_t lo,
                    std::size_t hi, LessThan lessThan) {
    if (hi - lo < 2) return;  // empty or single element: already sorted
    const std::size_t mid = lo + (hi - lo) / 2;
    mergeSortRange(items, scratch, lo, mid, lessThan);
    mergeSortRange(items, scratch, mid, hi, lessThan);
    // If already sorted, skip merge pass.
    if (!lessThan(items[mid], items[mid - 1])) return;
    mergeRuns(items, scratch, lo, mid, hi, lessThan);
}

}  // namespace detail

// Merge sort: recursive divide-and-conquer using scratch buffer.
template <typename T, typename LessThan>
void mergeSort(std::vector<T>& items, LessThan lessThan) {
    if (items.size() < 2) return;
    std::vector<T> scratch(items.size());
    detail::mergeSortRange(items, scratch, 0, items.size(), lessThan);
}

}  // namespace scapms::controller
