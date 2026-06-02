// ============================================================================
//  simulation.cpp  --  CARPOOL APP  --  time tick, car movement, ambient spawn
// ============================================================================

#include "simulation.h"

#include <cstdlib>   // rand
#include <sstream>

using namespace std;

// ---------------------------------------------------------------------------
//  helper: remove every still-in-queue slot belonging to this driver.
//   when a car departs, its un-booked empty seats must leave the pool too --
//   you can't book a seat on a car that has already left. (The seats come
//   back, all 3 of them, via completeTrip when the car returns home.)
// ---------------------------------------------------------------------------
static void purgeDriverSeats(queue<SeatSlot>& q, int driverId) {
    queue<SeatSlot> keep;
    while (!q.empty()) {
        SeatSlot s = q.front();
        q.pop();
        if (s.driverId != driverId) keep.push(s);
    }
    q.swap(keep);
}

// ---------------------------------------------------------------------------
//  departDriver  --  collect the car's unique destinations, route through all
//   of them greedily, and switch the car to TRAVELLING from km 0.
// ---------------------------------------------------------------------------
void departDriver(Driver& driver, const Graph& graph) {
    // unique destinations across all filled seats
    vector<int> dests;
    for (int s = 0; s < SEATS; ++s) {
        if (!driver.seats[s].filled) continue;
        int d = driver.seats[s].passengerDest;
        bool seen = false;
        for (size_t k = 0; k < dests.size(); ++k) {
            if (dests[k] == d) { seen = true; break; }
        }
        if (!seen) dests.push_back(d);
    }

    MultiRouteResult r = multiRoute(graph, driver.currentCity, dests);

    driver.route.assign(r.fullPath.begin(), r.fullPath.end());
    driver.totalKm     = r.totalKm;
    driver.travelledKm = 0;
    driver.status      = TRAVELLING;
}

// ---------------------------------------------------------------------------
//  completeTrip  --  trip done: arrive, drop everyone, return all seats,
//   head home, bump the rides-served counter.
// ---------------------------------------------------------------------------
void completeTrip(Driver& driver, queue<SeatSlot>& seatQueue) {
    if (!driver.route.empty()) {
        driver.currentCity = driver.route.back();   // final stop on the route
    }

    // empty every seat...
    for (int s = 0; s < SEATS; ++s) {
        driver.seats[s].filled          = false;
        driver.seats[s].passengerSource = 0;
        driver.seats[s].passengerDest   = 0;
        driver.seats[s].customerId      = 0;
    }
    // ...and return all of them to the queue (all unfilled now -> all 3 pushed)
    returnSeats(seatQueue, driver);

    driver.currentCity      = driver.homeCity;   // back to base
    driver.route.clear();
    driver.totalKm          = 0;
    driver.travelledKm      = 0;
    driver.status           = EMPTY;
    driver.ridesServedToday += 1;
}

// ---------------------------------------------------------------------------
//  tick  --  one real second of simulation.
// ---------------------------------------------------------------------------
void tick(Driver drivers[], queue<SeatSlot>& seatQueue,
          double& simTime, const Graph& graph) {
    simTime += TIME_SCALE;

    for (size_t i = 0; i < DRIVERS; ++i) {
        Driver& d = drivers[i];

        if (d.status == WAITING) {
            double waited = simTime - d.waitStartTime;
            if (countFilled(d) == SEATS) {
                purgeDriverSeats(seatQueue, d.id);      // (no-op: all 3 booked)
                departDriver(d, graph);                 // full -> go now
            } else if (waited >= WAIT_TIMEOUT) {
                if (countFilled(d) >= 1) {
                    purgeDriverSeats(seatQueue, d.id);  // drop its un-booked seats
                    departDriver(d, graph);             // partial -> go
                } else {
                    d.status        = EMPTY;             // nobody came -> cancel
                    d.waitStartTime = 0;                 // reset timer
                    // seats are still in queue (were never popped for 0 riders)
                }
            }
        } else if (d.status == TRAVELLING) {
            d.travelledKm += SPEED * TIME_SCALE;        // +5 km / real second
            if (d.travelledKm >= d.totalKm) {
                completeTrip(d, seatQueue);
            }
        }
    }
}

// ---------------------------------------------------------------------------
//  makeRandomCustomer  --  random source 1..DRIVERS, random different dest.
//   id comes from the caller's counter (start 1000) so ambient riders never
//   clash with the real user's id. srand() is NOT called here (done once in main).
// ---------------------------------------------------------------------------
Customer makeRandomCustomer(int& nextCustId) {
    Customer c;
    c.id         = nextCustId++;
    c.sourceCity = rand() % DRIVERS + 1;        // 1..DRIVERS
    do {
        c.destCity = rand() % DRIVERS + 1;      // loop until different
    } while (c.destCity == c.sourceCity);

    ostringstream nm;
    nm << "Rider" << c.id;
    c.name = nm.str();
    return c;
}

// ---------------------------------------------------------------------------
//  maybeSpawnRider  --  called once per tick. Accumulate spawnTimer; when it
//   reaches SPAWN_INTERVAL, reset it and (SPAWN_CHANCE% of the time) drop a
//   random rider into the SAME FIFO assignSeat() that tick() already watches.
//   The queue picks the driver -- no manual targeting -- so sometimes the
//   user's own car gets a seatmate, sometimes another car does.
// ---------------------------------------------------------------------------
void maybeSpawnRider(queue<SeatSlot>& seatQueue, Driver drivers[],
                     double simTime, double& spawnTimer, int& nextCustId) {

    // 1. Is anyone waiting? (boost spawn rate to fill their car faster)
    bool anyoneWaiting = false;
    for (int i = 0; i < DRIVERS; ++i) {
        if (drivers[i].status == WAITING) { anyoneWaiting = true; break; }
    }

    spawnTimer += TIME_SCALE;
    double currentInterval = anyoneWaiting ? 2.0 : SPAWN_INTERVAL;
    if (spawnTimer < currentInterval) return;
    spawnTimer = 0;

    int currentChance = anyoneWaiting ? 100 : SPAWN_CHANCE;
    if ((rand() % 100) >= currentChance) return;
    if (seatQueue.empty()) return;

    Driver* targetDriver = nullptr;

    if (anyoneWaiting) {
        // ── WAITING MODE: rotate queue to a WAITING car's slot ──
        int maxRotations = (int)seatQueue.size();
        while (maxRotations-- > 0) {
            SeatSlot front = seatQueue.front();
            Driver* d = nullptr;
            for (int i = 0; i < DRIVERS; ++i) {
                if (drivers[i].id == front.driverId) { d = &drivers[i]; break; }
            }
            if (d && d->status == WAITING) { targetDriver = d; break; }
            seatQueue.pop();
            seatQueue.push(front);
        }
        if (!targetDriver) return;

        // Spawn ONE rider to fill a waiting car's next seat
        int src = targetDriver->currentCity;
        int dst = src;
        while (dst == src) dst = rand() % DRIVERS + 1;
        Customer c;
        c.id = nextCustId++; c.sourceCity = src; c.destCity = dst;
        c.name = "rider#" + to_string(c.id);
        assignSeat(seatQueue, c, drivers, simTime);

    } else {
        // ── IDLE MODE: fill MULTIPLE random empty cars simultaneously ──

        // Collect all empty drivers and shuffle them (Fisher-Yates)
        Driver* emptyList[DRIVERS];
        int numEmpty = 0;
        for (int i = 0; i < DRIVERS; ++i) {
            if (drivers[i].status == EMPTY) emptyList[numEmpty++] = &drivers[i];
        }
        if (numEmpty == 0) return;

        for (int i = numEmpty - 1; i > 0; --i) {
            int j = rand() % (i + 1);
            Driver* tmp = emptyList[i]; emptyList[i] = emptyList[j]; emptyList[j] = tmp;
        }

        // Pick how many cars to fill at once: 1 .. numEmpty
        int carsToFill = (rand() % numEmpty) + 1;

        for (int ci = 0; ci < carsToFill; ++ci) {
            Driver* target = emptyList[ci];

            // Skip if this car already became WAITING mid-loop (boarded earlier)
            if (target->status != EMPTY) continue;

            // Randomly fill 1, 2, or all 3 seats for this car
            int seatsToFill = (rand() % SEATS) + 1;
            int src = target->currentCity;

            for (int r = 0; r < seatsToFill; ++r) {
                // Rotate queue to bring this driver's next free slot to the front
                bool found = false;
                int maxRot = (int)seatQueue.size();
                while (maxRot-- > 0) {
                    SeatSlot front = seatQueue.front();
                    if (front.driverId == target->id) { found = true; break; }
                    seatQueue.pop();
                    seatQueue.push(front);
                }
                if (!found) break;   // no more queue slots for this driver

                int dst = src;
                while (dst == src) dst = rand() % DRIVERS + 1;
                Customer c;
                c.id = nextCustId++; c.sourceCity = src; c.destCity = dst;
                c.name = "rider#" + to_string(c.id);
                assignSeat(seatQueue, c, drivers, simTime);
            }
        }
    }
}
