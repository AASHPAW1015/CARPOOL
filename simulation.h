// ============================================================================
//  simulation.h  --  CARPOOL APP  --  time tick, car movement, ambient spawn
// ============================================================================
//
//  The lead's integration core (the math half). One tick() = one real second:
//  it advances sim-time, makes WAITING cars depart (full, or on the 10-min
//  timeout), and moves TRAVELLING cars 5 km until they complete.
//
//  departDriver computes only the ROUTE (dests -> multiRoute). Fares are
//  computed in main at payment time (segmentFares), so this layer does not
//  depend on the payment stack and ambient cars never touch it.
//
// ============================================================================

#ifndef SIMULATION_H
#define SIMULATION_H

#include <vector>
#include <queue>
#include "types.h"        // Driver, Customer, SeatSlot (frozen)
#include "config.h"       // SPEED, TIME_SCALE, SEATS, DRIVERS, WAIT_TIMEOUT, SPAWN_*
#include "graph.h"        // Graph
#include "routing.h"      // multiRoute
#include "seatqueue.h"    // countFilled, returnSeats, assignSeat

using namespace std;

// --- one real second of the world: advance time, depart/move/complete cars ---
void tick(Driver drivers[], queue<SeatSlot>& seatQueue,
          double& simTime, const Graph& graph);

// --- lock in the route for a departing car (dests -> greedy multiRoute) ---
void departDriver(Driver& driver, const Graph& graph);

// --- end a trip: drop riders, return all seats, go home, +1 ride served ---
void completeTrip(Driver& driver, queue<SeatSlot>& seatQueue);

// --- a random rider: random source city, random different destination.
//     id comes from the caller's counter (start 1000) so it never clashes
//     with the real user's id; nextCustId is advanced. ---
Customer makeRandomCustomer(int& nextCustId);

// --- ambient life: accumulate spawnTimer by TIME_SCALE each tick; when it
//     reaches SPAWN_INTERVAL, reset it and (SPAWN_CHANCE% of the time) make a
//     random rider and feed them into the SAME FIFO assignSeat() that tick()
//     watches. The queue decides which driver gets the seat -- no targeting. ---
void maybeSpawnRider(queue<SeatSlot>& seatQueue, Driver drivers[],
                     double simTime, double& spawnTimer, int& nextCustId);

#endif // SIMULATION_H
