// ============================================================================
//  sorting.h  --  CARPOOL APP  --  3 sort algorithms + timing comparison
// ============================================================================
//
//  P3's island. Sort the driver array by a chosen field (criteria string).
//  Three classic algorithms so we can compare them in the report/viva:
//    merge     O(n log n)
//    quick     O(n log n) avg, O(n^2) worst
//    insertion O(n^2)
//
//  criteria: "rides" -> sort by ridesServedToday; anything else -> by id.
//  All sorts are non-decreasing and break ties by id, so the three produce
//  the IDENTICAL order on the same input.
//
// ============================================================================

#ifndef SORTING_H
#define SORTING_H

#include <vector>
#include <string>
#include "types.h"   // Driver (frozen)

using namespace std;

// --- sort drivers in place, non-decreasing by criteria (ties broken by id) ---
void mergeSortDrivers(vector<Driver>& drivers, const string& criteria);
void quickSortDrivers(vector<Driver>& drivers, const string& criteria);
void insertionSortDrivers(vector<Driver>& drivers, const string& criteria);

// --- time all three on copies of the same input, print a Big-O table ---
void compareTimings(const Driver drivers[]);

#endif // SORTING_H
