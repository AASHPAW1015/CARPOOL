// ============================================================================
//  seatqueue.cpp  --  CARPOOL APP  --  FIFO seat allocation queue
// ============================================================================

#include "seatqueue.h"

using namespace std;

// ---------------------------------------------------------------------------
//  helper: find a driver in the vector by its id (NULL if not present)
// ---------------------------------------------------------------------------
static Driver* findDriverById(Driver drivers[], int id) {
    for (int i = 0; i < DRIVERS; ++i) {
        if (drivers[i].id == id) return &drivers[i];
    }
    return NULL;
}

// ---------------------------------------------------------------------------
//  countFilled  --  how many seats are filled right now.
// ---------------------------------------------------------------------------
int countFilled(const Driver& driver) {
    int n = 0;
    for (int s = 0; s < SEATS; ++s) {
        if (driver.seats[s].filled) ++n;
    }
    return n;
}

// ---------------------------------------------------------------------------
//  initQueue  --  every seat of every driver, in driver-then-seat order.
//   size == DRIVERS * SEATS (15 with the frozen config).
// ---------------------------------------------------------------------------
queue<SeatSlot> initQueue(const Driver drivers[]) {
    queue<SeatSlot> q;
    for (int i = 0; i < DRIVERS; ++i) {
        for (int s = 0; s < SEATS; ++s) {
            SeatSlot slot;
            slot.driverId   = drivers[i].id;
            slot.seatNumber = s;
            q.push(slot);
        }
    }
    return q;
}

// ---------------------------------------------------------------------------
//  assignSeat  --  pop the front slot, seat the customer there.
//   FIFO fairness: whoever books first gets the next free seat.
//   On the driver's first filled seat, start the wait timer (status+stamp).
// ---------------------------------------------------------------------------
bool assignSeat(queue<SeatSlot>& seatQueue, const Customer& customer,
                Driver drivers[], double simTime) {
    if (seatQueue.empty()) return false;

    SeatSlot slot = seatQueue.front();
    seatQueue.pop();

    Driver* d = findDriverById(drivers, slot.driverId);
    if (d == NULL) return false;        // shouldn't happen; defensive

    bool wasEmpty = (countFilled(*d) == 0);

    Seat& seat = d->seats[slot.seatNumber];
    seat.filled          = true;
    seat.passengerSource = customer.sourceCity;
    seat.passengerDest   = customer.destCity;
    seat.customerId      = customer.id;

    // first passenger aboard -> begin the departure countdown
    if (wasEmpty) {
        d->status        = WAITING;
        d->waitStartTime = simTime;
    }
    return true;
}

// ---------------------------------------------------------------------------
//  returnSeats  --  push the driver's UNFILLED seats back to the rear.
//   used when a half-empty car departs (its empty seats rejoin the pool).
// ---------------------------------------------------------------------------
void returnSeats(queue<SeatSlot>& seatQueue, const Driver& driver) {
    for (int s = 0; s < SEATS; ++s) {
        if (!driver.seats[s].filled) {
            SeatSlot slot;
            slot.driverId   = driver.id;
            slot.seatNumber = s;
            seatQueue.push(slot);
        }
    }
}
