// ============================================================================
//  main.cpp  --  CARPOOL APP  --  STATE MACHINE (UI REBUILD V2)
// ============================================================================
//
//  States: HOME → MAP_SELECT_DEST → SEAT_WAIT → RIDE_SIM → PAYMENT → HOME
//          HOME → LEADERBOARD → HOME
//
//  Home screen uses typed input (cooked mode / readLine).
//  Destination select uses raw arrow keys.
//  Seat/Ride screens are non-blocking tick-driven.
//  Payment/Leaderboard use single-keypress via temporary raw mode.
//
//  DO NOT touch: config.h, types.h, graph.cpp/h, routing.cpp/h,
//  leaderboard.cpp/h, datastore.cpp/h, seatqueue.cpp/h,
//  paymentstack.cpp/h, sorting.cpp/h, searching.cpp/h, simulation.cpp/h
//
// ============================================================================

#include <vector>
#include <queue>
#include <stack>
#include <list>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <iostream>

#include "types.h"
#include "config.h"
#include "graph.h"
#include "routing.h"
#include "datastore.h"
#include "seatqueue.h"
#include "paymentstack.h"
#include "simulation.h"
#include "leaderboard.h"
#include "sorting.h"
#include "ui.h"

using namespace std;

// ── states ──
enum AppState {
    HOME,
    MAP_SELECT_DEST,
    SEAT_WAIT,
    RIDE_SIM,
    PAYMENT,
    LEADERBOARD
};

// ── globals ──
static Driver                drivers[DRIVERS];
static queue<SeatSlot>  seatQueue;
static stack<Payment>   payStack;
static list<Booking>    history;
static Graph                 graph;
static double                simTime    = 0;
static double                spawnTimer = 0;
static int                   nextCustId = 1000;
static AppState              state      = HOME;
static bool                  running    = true;

// per-booking state
static int    myDriverId    = -1;
static int    mySeatNum     = -1;
static int    myCustomerId  = -1;
static double myFare        = 0;
static int    sourceCity    = 0;
static int    nextBookingId = 1;

// destination-select navigation: cursor city id (1..5)
static int destCursor = 0;

// flag to track if fare was already computed for current trip
static bool fareComputed = false;

// ── navigation map for dest select (UI neighbours, NOT graph edges) ──
static int navMove(int current, Key dir) {
    switch (current) {
        case 5: // City 5 (top-left)
            if (dir == KEY_DOWN)  return 2;
            if (dir == KEY_RIGHT) return 3;
            break;
        case 2: // City 2 (mid-left)
            if (dir == KEY_UP)    return 5;
            if (dir == KEY_DOWN)  return 1;
            if (dir == KEY_RIGHT) return 3;
            break;
        case 1: // City 1 (bottom-left)
            if (dir == KEY_UP)    return 2;
            if (dir == KEY_RIGHT) return 3;
            break;
        case 3: // City Mall (mid-right)
            if (dir == KEY_DOWN)  return 4;
            if (dir == KEY_LEFT)  return 5;
            if (dir == KEY_UP)    return 5;
            break;
        case 4: // City 4 (bottom-right)
            if (dir == KEY_UP)    return 3;
            if (dir == KEY_LEFT)  return 1;
            break;
    }
    return current;
}

// ── find a driver by id ──
static Driver* findDriver(int id) {
    for (size_t i = 0; i < DRIVERS; ++i) {
        if (drivers[i].id == id) return &drivers[i];
    }
    return NULL;
}

// ── book user seat: find a car at sourceCity, fill one seat ──
// Uses the FIFO queue fairly but targets the source city.
static bool bookUserSeat(const Customer& cust) {
    queue<SeatSlot> keep;
    bool booked = false;

    while (!seatQueue.empty()) {
        SeatSlot s = seatQueue.front();
        seatQueue.pop();

        if (!booked) {
            Driver* d = findDriver(s.driverId);
            if (d && d->currentCity == cust.sourceCity &&
                d->status != TRAVELLING && !d->seats[s.seatNumber].filled) {

                bool wasEmpty = (countFilled(*d) == 0);
                Seat& seat = d->seats[s.seatNumber];
                seat.filled          = true;
                seat.passengerSource = cust.sourceCity;
                seat.passengerDest   = cust.destCity;
                seat.customerId      = cust.id;
                if (wasEmpty) {
                    d->status        = WAITING;
                    d->waitStartTime = simTime;
                }
                myDriverId = d->id;
                mySeatNum  = s.seatNumber;
                booked     = true;
                continue;   // drop this slot from queue
            }
        }
        keep.push(s);
    }
    seatQueue.swap(keep);
    return booked;
}

// ── compute fare at departure (before completeTrip wipes seats) ──
static double chargeUser(const Driver& d) {
    vector<Customer> pax;
    for (int s = 0; s < SEATS; ++s) {
        if (!d.seats[s].filled) continue;
        Customer p;
        p.id         = d.seats[s].customerId;
        p.sourceCity = d.seats[s].passengerSource;
        p.destCity   = d.seats[s].passengerDest;
        pax.push_back(p);
    }

    vector<int> fullPath(d.route.begin(), d.route.end());
    vector<double> fares = segmentFares(fullPath, graph, pax);

    double fare = 0;
    for (size_t i = 0; i < pax.size(); ++i) {
        if (pax[i].id == myCustomerId) fare = fares[i];
    }

    vector<FareItem> items;
    FareItem f; f.customerId = myCustomerId; f.amount = fare;
    items.push_back(f);
    pushPayments(payStack, items);
    return fare;
}

// ── getchar but without requiring Enter (temporary raw mode) ──
static char getSingleChar() {
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    newt.c_cc[VMIN] = 1;
    newt.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &newt);
    char c;
    read(STDIN_FILENO, &c, 1);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &oldt);
    return c;
}

// ========================================================================
//  STATE HANDLERS
// ========================================================================

// non-blocking peek: does stdin have a complete line ready?
// in cooked (canonical) mode the kernel holds input until Enter is pressed,
// so select() returns true only when a full line is available — getline
// will not block after this returns true.
static bool inputAvailable() {
    struct timeval tv = {0, 0};   // zero timeout = just peek
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
}


static void handleHomeInput(const string& input) {
    if (input == "city 1") {
        sourceCity = 1; destCursor = 3; state = MAP_SELECT_DEST;
    } else if (input == "city 2") {
        sourceCity = 2; destCursor = 3; state = MAP_SELECT_DEST;
    } else if (input == "city mall") {
        sourceCity = 3; destCursor = 1; state = MAP_SELECT_DEST;
    } else if (input == "city 4") {
        sourceCity = 4; destCursor = 3; state = MAP_SELECT_DEST;
    } else if (input == "city 5") {
        sourceCity = 5; destCursor = 1; state = MAP_SELECT_DEST;
    } else if (input == "city circle") {
        printf("  City Circle is a junction \xe2\x80\x94 not selectable.\n");
        fflush(stdout);
    } else if (input == "leaderboard") {
        state = LEADERBOARD;
    } else if (input == "q" || input == "quit") {
        running = false;
    } else if (!input.empty()) {
        printf("  unknown city. try: city 1, city 2, city mall, city 4, city 5\n");
        fflush(stdout);
    }
}

static void doHome() {
    static AppState lastHomeState = (AppState)-1;
    
    // If we just entered the HOME state, draw the full screen
    if (lastHomeState != HOME) {
        drawHome(drivers, simTime);
        lastHomeState = HOME;
    } else {
        // Just update the map part, leaving the prompt intact so typing isn't cleared
        drawHomeUpdate(drivers, simTime);
    }

    if (inputAvailable()) {
        string input = readLine();
        handleHomeInput(input);
        lastHomeState = (AppState)-1; // reset for next time
    }
}

static void doMapSelect() {
    enableRawMode();

    while (state == MAP_SELECT_DEST && running) {
        drawMapSelect(destCursor, sourceCity);

        // wait for key, ticking sim periodically
        int waitLoops = 0;
        Key k = KEY_NONE;
        while (k == KEY_NONE) {
            usleep(50000);  // 50ms
            waitLoops++;
            // tick sim every ~1 second (20 * 50ms = 1000ms)
            if (waitLoops % 20 == 0) {
                tick(drivers, seatQueue, simTime, graph);
                maybeSpawnRider(seatQueue, drivers, simTime, spawnTimer, nextCustId);
            }
            k = readKey();
        }

        if (k == KEY_UP || k == KEY_DOWN || k == KEY_LEFT || k == KEY_RIGHT) {
            int next = navMove(destCursor, k);
            // skip source city
            if (next == sourceCity) {
                int after = navMove(next, k);
                if (after != sourceCity) next = after;
                else next = destCursor;  // can't move there
            }
            destCursor = next;
        } else if (k == KEY_ENTER) {
            if (destCursor == sourceCity) {
                // "pick a different city" — stay on screen
            } else {
                disableRawMode();

                myCustomerId = nextCustId++;
                Customer c;
                c.id = myCustomerId;
                c.sourceCity = sourceCity;
                c.destCity = destCursor;
                c.name = "You";

                bool ok = bookUserSeat(c);
                if (!ok) {
                    printf("  \033[31mNo seats available right now. Try again.\033[0m\n");
                    fflush(stdout);
                    usleep(2000000);
                    state = HOME;
                } else {
                    fareComputed = false;
                    state = SEAT_WAIT;
                }
                return;
            }
        } else if (k == KEY_QUIT) {
            disableRawMode();
            state = HOME;
            return;
        }
    }
    disableRawMode();
}

static void doSeatWait() {
    Driver* d = findDriver(myDriverId);
    if (!d) { state = HOME; return; }

    drawSeatScreen(*d, mySeatNum, simTime);

    // check if driver departed → compute fare and switch to ride
    if (d->status == TRAVELLING && !fareComputed) {
        myFare = chargeUser(*d);
        fareComputed = true;
        state = RIDE_SIM;
    }
}

static void doRideSim() {
    Driver* d = findDriver(myDriverId);
    if (!d) { state = HOME; return; }

    drawRideSim(*d, graph);

    // check if trip completed
    if (d->travelledKm >= d->totalKm || d->status != TRAVELLING) {
        state = PAYMENT;
    }
}

static void doPayment() {
    drawPayment(myFare, myDriverId);

    // single keypress: P to pay, S to skip
    char c = getSingleChar();

    if (c == 'p' || c == 'P') {
        processNext(payStack);
        printf("  \033[1;32m\xe2\x9c\x93 payment received. thanks!\033[0m\n");
        fflush(stdout);
        usleep(1000000);

        Booking b;
        b.bookingId  = nextBookingId++;
        b.customerId = myCustomerId;
        b.driverId   = myDriverId;
        b.seatNumber = mySeatNum;
        b.fare       = myFare;
        b.active     = true;
        addToHistory(history, b);
        state = HOME;
    } else if (c == 's' || c == 'S') {
        processNext(payStack);
        printf("  skipped.\n");
        fflush(stdout);
        usleep(1000000);

        Booking b;
        b.bookingId  = nextBookingId++;
        b.customerId = myCustomerId;
        b.driverId   = myDriverId;
        b.seatNumber = mySeatNum;
        b.fare       = myFare;
        b.active     = false;
        addToHistory(history, b);
        state = HOME;
    }
    // else: ignore, doPayment will be called again next tick
}

static void doLeaderboard() {
    drawLeaderboard(drivers);

    // any key returns to home
    getSingleChar();
    state = HOME;
}

// ============================================================================
//  main
// ============================================================================
int main() {
    srand((unsigned)time(0));   // ONCE, here, never again

    graph = buildGraph();

    bool loaded = loadDrivers(drivers);
    if (!loaded) {
        initDrivers(drivers);
    }

    seatQueue = initQueue(drivers);
    history   = loadHistory();

    while (running) {
        // advance simulation
        tick(drivers, seatQueue, simTime, graph);
        maybeSpawnRider(seatQueue, drivers, simTime, spawnTimer, nextCustId);

        switch (state) {
            case HOME:            doHome();         break;
            case MAP_SELECT_DEST: doMapSelect();    break;
            case SEAT_WAIT:       doSeatWait();     break;
            case RIDE_SIM:        doRideSim();      break;
            case PAYMENT:         doPayment();      break;
            case LEADERBOARD:     doLeaderboard();  break;
        }

        usleep(800000);  // ~0.8 real second per tick
    }

    // shutdown
    saveDrivers(drivers);
    saveHistory(history);
    printf("\033[0m\033[?25h");
    printf("\nThanks for riding CARPOOL!  Goodbye.\n");
    return 0;
}
