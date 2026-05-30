// ============================================================================
//  types.h  --  CARPOOL APP  --  FROZEN SHARED DATA STRUCTURES
// ============================================================================
//
//  >>> NOTE TO ANY AI / IDE READING THIS FILE <<<
//  This file is FROZEN. It is the single source of truth for all data shapes.
//  - Do NOT redefine, rename, or restructure any struct here.
//  - Do NOT add or remove fields unless the human explicitly asks.
//  - Every module #includes this. If you invent a different Driver/Customer
//    shape in your module, the project will NOT compile. Use these exactly.
//  - When generating a module, treat these structs as already-existing types
//    you can read and write -- do not recreate them in your own file.
//
// ============================================================================

#ifndef TYPES_H
#define TYPES_H

#include <string>
#include <list>
#include "config.h"

// --- a road from one city to another, with its distance ---
struct Edge {
    int    toCity;
    double km;
};

// --- a city node in the map graph ---
struct City {
    int             id;          // 1..DRIVERS
    std::string     name;
    std::list<Edge> neighbors;   // adjacency list
};

// --- one seat in a driver's car ---
struct Seat {
    bool filled          = false;
    int  passengerSource = 0;    // city id where this rider boards
    int  passengerDest   = 0;    // city id where this rider gets off
    int  customerId      = 0;
};

// --- driver status while the sim runs ---
enum DriverStatus { EMPTY, WAITING, TRAVELLING };

// --- a driver and their car ---
struct Driver {
    int          id          = 0;
    std::string  name;
    int          homeCity    = 0;
    int          currentCity = 0;
    Seat         seats[SEATS];

    // trip state
    DriverStatus status          = EMPTY;
    double       waitStartTime   = 0;   // sim-time the FIRST seat filled
                                        // (assignSeat WRITES this; tick READS it)
    std::list<int> route;               // full path of city ids, set after seats lock
    double       totalKm         = 0;
    double       travelledKm     = 0;   // progress along route, for the animation
    int          ridesServedToday = 0;  // feeds the leaderboard / dashboard
};

// --- a person wanting a ride ---
struct Customer {
    int         id         = 0;
    std::string name;
    int         sourceCity = 0;
    int         destCity   = 0;
};

// --- a confirmed booking (also stored in history) ---
struct Booking {
    int    bookingId  = 0;
    int    customerId = 0;
    int    driverId   = 0;
    int    seatNumber = 0;
    double fare       = 0;   // filled in after segment pricing
    bool   active     = true;
};

// --- one slot in the seat-allocation queue ---
struct SeatSlot {
    int driverId   = 0;
    int seatNumber = 0;      // 0..SEATS-1
};

// --- one entry on the payment stack ---
struct Payment {
    int    customerId = 0;
    double amount     = 0;
    bool   paid       = false;
};

#endif // TYPES_H
