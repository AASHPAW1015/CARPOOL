// ============================================================================
//  search.h  --  CARPOOL APP  --  linear + binary search over drivers
// ============================================================================
//
//  P3's island. Two searches:
//    linearSearchByCity -- O(n) scan, collects ALL drivers at a city.
//    binarySearchById   -- O(log n), but PRECONDITION: array sorted by id.
//
// ============================================================================

#ifndef SEARCH_H
#define SEARCH_H

#include <vector>
#include "types.h"   // Driver (frozen)

using namespace std;

// --- indices of every driver currently parked at `city` (empty if none) ---
vector<int> linearSearchByCity(const vector<Driver>& drivers, int city);

// --- binary search for a driver by id; returns pointer or NULL.
//     PRECONDITION: `sortedDrivers` is sorted ascending by id ---
Driver* binarySearchById(vector<Driver>& sortedDrivers, int id);

#endif // SEARCH_H
