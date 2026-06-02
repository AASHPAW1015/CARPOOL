// ============================================================================
//  sorting.cpp  --  CARPOOL APP  --  3 sort algorithms + timing comparison
// ============================================================================

#include "sorting.h"

#include <cstdio>
#include <ctime>

using namespace std;

// ---------------------------------------------------------------------------
//  the sort key for a driver, given the criteria string.
//   "rides" -> ridesServedToday ; anything else -> id.
// ---------------------------------------------------------------------------
static long sortKey(const Driver& d, const string& criteria) {
    if (criteria == "rides") return (long)d.ridesServedToday;
    return (long)d.id;
}

// "does a come strictly before b?"  primary = key, tie-break = id.
// the id tie-break makes all three algorithms agree exactly on equal keys.
static bool driverLess(const Driver& a, const Driver& b, const string& criteria) {
    long ka = sortKey(a, criteria);
    long kb = sortKey(b, criteria);
    if (ka != kb) return ka < kb;
    return a.id < b.id;
}

// ===========================================================================
//  MERGE SORT  --  O(n log n) always; stable; needs a temp buffer.
// ===========================================================================
static void mergeRange(vector<Driver>& v, vector<Driver>& tmp,
                       int lo, int mid, int hi, const string& criteria) {
    int i = lo, j = mid + 1, k = lo;
    while (i <= mid && j <= hi) {
        if (!driverLess(v[j], v[i], criteria))   // v[i] <= v[j] -> take left (stable)
            tmp[k++] = v[i++];
        else
            tmp[k++] = v[j++];
    }
    while (i <= mid) tmp[k++] = v[i++];
    while (j <= hi)  tmp[k++] = v[j++];
    for (int t = lo; t <= hi; ++t) v[t] = tmp[t];
}

static void mergeSortRec(vector<Driver>& v, vector<Driver>& tmp,
                         int lo, int hi, const string& criteria) {
    if (lo >= hi) return;
    int mid = lo + (hi - lo) / 2;
    mergeSortRec(v, tmp, lo, mid, criteria);
    mergeSortRec(v, tmp, mid + 1, hi, criteria);
    mergeRange(v, tmp, lo, mid, hi, criteria);
}

void mergeSortDrivers(vector<Driver>& drivers, const string& criteria) {
    if (drivers.size() < 2) return;
    vector<Driver> tmp(drivers.size());
    mergeSortRec(drivers, tmp, 0, (int)drivers.size() - 1, criteria);
}

// ===========================================================================
//  QUICK SORT  --  O(n log n) avg, O(n^2) worst; in place.
//   middle-element pivot keeps already-sorted input out of the worst case.
// ===========================================================================
static void quickRec(vector<Driver>& v, int lo, int hi,
                     const string& criteria) {
    if (lo >= hi) return;
    Driver pivot = v[lo + (hi - lo) / 2];   // middle pivot
    int i = lo, j = hi;
    while (i <= j) {
        while (driverLess(v[i], pivot, criteria)) ++i;
        while (driverLess(pivot, v[j], criteria)) --j;
        if (i <= j) {
            Driver t = v[i]; v[i] = v[j]; v[j] = t;
            ++i; --j;
        }
    }
    quickRec(v, lo, j, criteria);
    quickRec(v, i, hi, criteria);
}

void quickSortDrivers(vector<Driver>& drivers, const string& criteria) {
    if (drivers.size() < 2) return;
    quickRec(drivers, 0, (int)drivers.size() - 1, criteria);
}

// ===========================================================================
//  INSERTION SORT  --  O(n^2); simple; great on tiny/nearly-sorted arrays.
// ===========================================================================
void insertionSortDrivers(vector<Driver>& drivers, const string& criteria) {
    for (size_t i = 1; i < drivers.size(); ++i) {
        Driver key = drivers[i];
        int j = (int)i - 1;
        while (j >= 0 && driverLess(key, drivers[j], criteria)) {
            drivers[j + 1] = drivers[j];
            --j;
        }
        drivers[j + 1] = key;
    }
}

// ===========================================================================
//  compareTimings  --  run all three on copies, print times + Big-O.
//   verifies all three produce the identical order.
// ===========================================================================
void compareTimings(const Driver drivers[]) {
    vector<Driver> a(drivers, drivers + DRIVERS);   // for merge
    vector<Driver> b(drivers, drivers + DRIVERS);   // for quick
    vector<Driver> c(drivers, drivers + DRIVERS);   // for insertion
    const string criteria = "rides";

    clock_t t0 = clock();
    mergeSortDrivers(a, criteria);
    clock_t t1 = clock();
    quickSortDrivers(b, criteria);
    clock_t t2 = clock();
    insertionSortDrivers(c, criteria);
    clock_t t3 = clock();

    double mergeMs = 1000.0 * (double)(t1 - t0) / CLOCKS_PER_SEC;
    double quickMs = 1000.0 * (double)(t2 - t1) / CLOCKS_PER_SEC;
    double insMs   = 1000.0 * (double)(t3 - t2) / CLOCKS_PER_SEC;

    bool identical = (a.size() == b.size() && b.size() == c.size());
    for (size_t i = 0; i < a.size() && identical; ++i) {
        if (a[i].id != b[i].id || b[i].id != c[i].id) identical = false;
    }

    printf("\n  Sort comparison (n = %d, by \"%s\")\n", DRIVERS, criteria.c_str());
    printf("  ---------------------------------------------------------\n");
    printf("  %-12s %-26s %10s\n", "algorithm", "complexity", "time (ms)");
    printf("  %-12s %-26s %10.4f\n", "merge",     "O(n log n)",                 mergeMs);
    printf("  %-12s %-26s %10.4f\n", "quick",     "O(n log n) avg, O(n^2) worst", quickMs);
    printf("  %-12s %-26s %10.4f\n", "insertion", "O(n^2)",                     insMs);
    printf("  ---------------------------------------------------------\n");
    printf("  all three produced identical order: %s\n\n", identical ? "YES" : "NO");
}
