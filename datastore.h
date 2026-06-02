// ============================================================================
//  datastore.h  --  CARPOOL APP  --  driver array, history list, file I/O
// ============================================================================
//
//  P1's island. Builds the DRIVERS drivers, keeps ride history as a linked
//  list (newest first), and saves/loads both to plain text files under data/.
//  Only persists the stable fields (id, name, home/current city, status,
//  ridesServedToday) -- live trip state (seats, route, timers) is rebuilt at
//  runtime, not stored.
//
// ============================================================================

#ifndef DATASTORE_H
#define DATASTORE_H

#include <vector>
#include <list>
#include "types.h"    // Driver, Booking (frozen)
#include "config.h"   // DRIVERS (frozen)

using namespace std;

// --- make the DRIVERS drivers, one stationed at each city 1..DRIVERS ---
void initDrivers(Driver drivers[]);

// --- prepend a booking to the history list (newest at the head) ---
void addToHistory(list<Booking>& history, const Booking& booking);

// --- persist / restore drivers to data/drivers.txt ---
bool saveDrivers(const Driver drivers[]);
bool loadDrivers(Driver drivers[]);

// --- persist / restore history to data/history.txt ---
bool saveHistory(const list<Booking>& history);
list<Booking> loadHistory();

#endif // DATASTORE_H
