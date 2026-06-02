// ============================================================================
//  search.cpp  --  CARPOOL APP  --  linear + binary search over drivers
// ============================================================================

#include "search.h"

using namespace std;

// ---------------------------------------------------------------------------
//  linearSearchByCity  --  O(n): scan all, collect indices at this city.
// ---------------------------------------------------------------------------
vector<int> linearSearchByCity(const vector<Driver>& drivers, int city) {
    vector<int> hits;
    for (size_t i = 0; i < drivers.size(); ++i) {
        if (drivers[i].currentCity == city) {
            hits.push_back((int)i);
        }
    }
    return hits;
}

// ---------------------------------------------------------------------------
//  binarySearchById  --  O(log n) classic binary search.
//   PRECONDITION: sortedDrivers is sorted ascending by id.
// ---------------------------------------------------------------------------
Driver* binarySearchById(vector<Driver>& sortedDrivers, int id) {
    int lo = 0;
    int hi = (int)sortedDrivers.size() - 1;

    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        int midId = sortedDrivers[mid].id;

        if (midId == id) return &sortedDrivers[mid];
        if (id < midId)  hi = mid - 1;
        else             lo = mid + 1;
    }
    return NULL;   // not found
}
