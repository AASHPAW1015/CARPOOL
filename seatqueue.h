// ============================================================================
//  seatqueue.h  --  CARPOOL APP  --  FIFO seat allocation queue
// ============================================================================
//
//  P2's island. Every seat of every driver goes into one queue at startup
//  (DRIVERS * SEATS slots). A booking pops the front slot -> fairness is
//  automatic (first come, first served). Unused / freed seats get pushed
//  back to the rear.
//
//  ONE wiring detail (see team context 4.2): assignSeat WRITES the driver's
//  status + waitStartTime when the FIRST seat fills; the lead's tick() READS
//  them to decide departure.
//
// ============================================================================

#ifndef SEATQUEUE_H
#define SEATQUEUE_H

#include <queue>
#include <vector>
#include "types.h"    // SeatSlot, Customer, Driver (frozen)
#include "config.h"   // SEATS, DRIVERS (frozen)

using namespace std;

// --- build the queue of all seats from all drivers (size DRIVERS*SEATS) ---
queue<SeatSlot> initQueue(const Driver drivers[]);

// --- pop the front slot and seat this customer; returns false if queue empty.
//     on a driver's FIRST filled seat: status=WAITING, waitStartTime=simTime ---
bool assignSeat(queue<SeatSlot>& seatQueue, const Customer& customer,
                Driver drivers[], double simTime);

// --- push every UNFILLED seat of this driver back to the rear of the queue ---
void returnSeats(queue<SeatSlot>& seatQueue, const Driver& driver);

// --- count how many of a driver's seats are currently filled (0..SEATS) ---
int countFilled(const Driver& driver);

#endif // SEATQUEUE_H
