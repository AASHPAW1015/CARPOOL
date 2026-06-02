// ============================================================================
//  datastore.cpp  --  CARPOOL APP  --  driver array, history list, file I/O
// ============================================================================

#include "datastore.h"

#include <fstream>
#include <sstream>
#include <string>
#include <sys/stat.h>

using namespace std;

static const char* DRIVERS_FILE = "data/drivers.txt";
static const char* HISTORY_FILE = "data/history.txt";

// ---------------------------------------------------------------------------
//  initDrivers  --  one driver parked at each city 1..DRIVERS, all idle.
// ---------------------------------------------------------------------------
void initDrivers(Driver drivers[]) {
    for (int i = 0; i < DRIVERS; ++i) {
        Driver& d = drivers[i];
        d.id          = i + 1;
        d.homeCity    = i + 1;
        d.currentCity = i + 1;
        d.status      = EMPTY;
        d.ridesServedToday = 0;

        ostringstream nm;
        nm << "Driver" << (i + 1);
        d.name = nm.str();

        for (int s = 0; s < SEATS; ++s) {
            d.seats[s].filled          = false;
            d.seats[s].passengerSource = 0;
            d.seats[s].passengerDest   = 0;
            d.seats[s].customerId      = 0;
        }
    }
}

// ---------------------------------------------------------------------------
//  addToHistory  --  newest booking goes to the head of the linked list.
// ---------------------------------------------------------------------------
void addToHistory(list<Booking>& history, const Booking& booking) {
    history.push_front(booking);
}

// ---------------------------------------------------------------------------
//  saveDrivers  --  one driver per line:
//     id name homeCity currentCity status ridesServedToday
// ---------------------------------------------------------------------------
bool saveDrivers(const Driver drivers[]) {
    mkdir("data", 0755);          // create data/ if it doesn't exist yet
    ofstream out(DRIVERS_FILE);
    if (!out) return false;

    for (int i = 0; i < DRIVERS; ++i) {
        const Driver& d = drivers[i];
        out << d.id << ' '
            << d.name << ' '
            << d.homeCity << ' '
            << d.currentCity << ' '
            << (int)d.status << ' '
            << d.ridesServedToday << '\n';
    }
    return out.good();
}

// ---------------------------------------------------------------------------
//  loadDrivers  --  rebuild drivers from data/drivers.txt (empty if no file).
//   live trip state (seats, route, timers) is reset to defaults here.
// ---------------------------------------------------------------------------
bool loadDrivers(Driver drivers[]) {
    ifstream in(DRIVERS_FILE);
    if (!in) return false;

    int count = 0;
    string line;
    while (getline(in, line) && count < DRIVERS) {
        if (line.empty()) continue;
        istringstream ss(line);

        Driver d;
        int statusInt = 0;
        ss >> d.id >> d.name >> d.homeCity >> d.currentCity
           >> statusInt >> d.ridesServedToday;
        if (ss.fail()) continue;

        d.status = (DriverStatus)statusInt;
        for (int s = 0; s < SEATS; ++s) {
            d.seats[s].filled          = false;
            d.seats[s].passengerSource = 0;
            d.seats[s].passengerDest   = 0;
            d.seats[s].customerId      = 0;
        }
        drivers[count++] = d;
    }
    return count == DRIVERS;
}

// ---------------------------------------------------------------------------
//  saveHistory  --  one booking per line:
//     bookingId customerId driverId seatNumber fare active
// ---------------------------------------------------------------------------
bool saveHistory(const list<Booking>& history) {
    mkdir("data", 0755);
    ofstream out(HISTORY_FILE);
    if (!out) return false;

    for (list<Booking>::const_iterator it = history.begin();
         it != history.end(); ++it) {
        const Booking& b = *it;
        out << b.bookingId << ' '
            << b.customerId << ' '
            << b.driverId << ' '
            << b.seatNumber << ' '
            << b.fare << ' '
            << (b.active ? 1 : 0) << '\n';
    }
    return out.good();
}

// ---------------------------------------------------------------------------
//  loadHistory  --  rebuild the booking list (preserves file order).
// ---------------------------------------------------------------------------
list<Booking> loadHistory() {
    list<Booking> history;
    ifstream in(HISTORY_FILE);
    if (!in) return history;

    string line;
    while (getline(in, line)) {
        if (line.empty()) continue;
        istringstream ss(line);

        Booking b;
        int activeInt = 1;
        ss >> b.bookingId >> b.customerId >> b.driverId
           >> b.seatNumber >> b.fare >> activeInt;
        if (ss.fail()) continue;

        b.active = (activeInt != 0);
        history.push_back(b);
    }
    return history;
}
