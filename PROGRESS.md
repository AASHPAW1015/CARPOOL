# Carpool App — Build Progress

## Files Complete
- [x] config.h       (frozen — already done)
- [x] types.h        (frozen — already done)
- [x] graph.cpp/h
- [x] routing.cpp/h
- [x] leaderboard.cpp/h
- [x] datastore.cpp/h
- [x] seatqueue.cpp/h    (written here; spec said P2-done but file was missing)
- [x] paymentstack.cpp/h (written here; spec said P2-done but file was missing)
- [x] sorting.cpp/h
- [x] search.cpp/h
- [x] simulation.cpp/h
- [x] ui.cpp/h
- [x] main.cpp

## Current Step
UI REBUILD V2 DONE. `g++ -std=c++11 *.cpp -o carpool` builds clean (zero warnings
with -Wall -Wextra). Rewrote ui.h, ui.cpp, and main.cpp to match the V2 spec:
- 6 screens: HOME, MAP_SELECT_DEST, SEAT_WAIT, RIDE_SIM, PAYMENT, LEADERBOARD
- Big ASCII map with per-city ANSI colours (red/green/yellow/blue/magenta/white)
- Terminal centering via ioctl(TIOCGWINSZ)
- Two input modes: typed (cooked) for HOME, raw arrow keys for dest select
- Car tags [C1]..[C5] with driver-specific colours
- Seat dots (●/○), progress bars (█/░), route arrows (──✓──> / ──●──>)
- Payment: P/S single-keypress, double-border Unicode box
- Leaderboard: BST → inorder → reverse → ranked table + compareTimings

## How to build & run
    cd <this folder>
    g++ -std=c++11 *.cpp -o carpool
    ./carpool
  Controls: type a city name at the HOME prompt (city 1 / city 2 / city mall /
  city 4 / city 5); arrow keys on the dest-select screen; P/S on payment;
  type "leaderboard" for rankings; type "q" to quit.
  Must run in a real terminal (uses termios raw mode + ANSI).
  data/ holds drivers.txt + history.txt (auto-saved on quit; safe to delete for
  a fresh start).

## Known Issues
- isOnSegment uses FIRST index of each city in fullPath. Fine for simple
  chained shortest paths (our routes don't revisit a city in the tests). If a
  route ever revisits a city, fare attribution could be off — revisit only if
  it shows up in practice.
- DEVIATION from literal spec pseudocode (intentional, documented): the spec's
  tick() calls returnSeats() on a partial departure, but the unfilled seats are
  still in the queue at that point, so pushing them back DUPLICATES slots and the
  queue grows/double-books over time. Fix: tick() purges the departing driver's
  leftover slots from the queue (purgeDriverSeats helper in simulation.cpp);
  completeTrip() returns all 3 seats when the car comes home. Net: queue stays
  balanced at 15. returnSeats() is still implemented + tested as a standalone
  utility (used by completeTrip via reset-then-return-all). Viva answer unchanged:
  "a departed car's seats become available again when it returns home."

## Context for next session
- graph.h defines `struct Graph { std::vector<City> cities; }` indexed by id
  (cities[1..6]; index 0 is an unused placeholder so id == vector index).
  CITIES = 6 (nodes 1..5 are cities with drivers, node 6 = City Circle junction).
- graph.h also defines `struct PathResult { std::vector<int> path; double distance; }`
  (distance == -1 and empty path when unreachable).
- API: `Graph buildGraph();`
        `PathResult dijkstra(const Graph&, int start, int end);`
        `bool bfsReachable(const Graph&, int start, int end);`
        `std::vector<std::vector<int> > dfsAllPaths(const Graph&, int start, int end);`
- Verified: dijkstra(1,3) -> path 1->6->3, 9km; bfsReachable(1,5) -> true.
- routing.h defines `struct MultiRouteResult { std::vector<int> fullPath; double totalKm; }`.
  API: `MultiRouteResult multiRoute(const Graph&, int startCity, const std::vector<int>& dests);`
       `std::vector<double> segmentFares(const std::vector<int>& fullPath, const Graph&, const std::vector<Customer>& passengers);`
       `bool isOnSegment(const Customer&, int cityA, int cityB, const std::vector<int>& fullPath);`
  segmentFares takes passengers as std::vector<Customer> (uses .sourceCity/.destCity);
  returns fares in the same order. edgeKm() is a static helper in routing.cpp.
- leaderboard.h defines `struct BSTNode { Driver driver; BSTNode* left; BSTNode* right; }`.
  API: `BSTNode* insertDriver(BSTNode* root, const Driver&);` (key=ridesServedToday, ties go right)
       `std::vector<Driver> inorder(BSTNode*);`  (ascending / DFS)
       `std::vector<Driver> levelOrder(BSTNode*);` (BFS, root first)
       `Driver* searchDriver(BSTNode*, int key);` (NULL if absent)
       `void freeTree(BSTNode*);`
- datastore.h API: `std::vector<Driver> initDrivers();` (names "Driver1".."Driver5", single token)
       `void addToHistory(std::list<Booking>&, const Booking&);` (push_front)
       `bool saveDrivers(const std::vector<Driver>&);` / `std::vector<Driver> loadDrivers();`
       `bool saveHistory(const std::list<Booking>&);` / `std::list<Booking> loadHistory();`
  File formats (space-separated, one record/line under data/):
    drivers.txt: id name homeCity currentCity status ridesServedToday
    history.txt: bookingId customerId driverId seatNumber fare active
  load* return empty if file missing; live trip state (seats/route/timers) reset on load.
- seatqueue.h API: `std::queue<SeatSlot> initQueue(const std::vector<Driver>&);`
       `bool assignSeat(std::queue<SeatSlot>&, const Customer&, std::vector<Driver>&, double simTime);`
       `void returnSeats(std::queue<SeatSlot>&, const Driver&);`
       `int countFilled(const Driver&);`
  assignSeat pops front slot, finds driver by id, fills seat; on FIRST filled seat
  sets status=WAITING + waitStartTime=simTime (the lead's tick reads these).
  countFilled is the shared helper used by tick().
- paymentstack.h defines `struct FareItem { int customerId; double amount; };`.
  API: `void pushPayments(std::stack<Payment>&, const std::vector<FareItem>&);`
       `Payment processNext(std::stack<Payment>&);` (pops top, sets paid=true; on
        empty returns sentinel with customerId==-1 -> use isSentinel(p) to detect)
       `void rollbackAll(std::stack<Payment>&);` (pops all, marks unpaid, empties)
       `bool isSentinel(const Payment&);`
  departDriver (Step 9) will build vector<FareItem> from segmentFares output +
  seat customerIds, then pushPayments onto the global payStack.
- sorting.h API: `void mergeSortDrivers(std::vector<Driver>&, const std::string& criteria);`
       (same for quickSortDrivers, insertionSortDrivers) + `void compareTimings(const std::vector<Driver>&);`
  criteria=="rides" -> sort by ridesServedToday, else by id; non-decreasing,
  TIE-BREAK BY id so all three give identical order. quick uses middle pivot
  (Hoare partition). compareTimings uses clock()/CLOCKS_PER_SEC.
- search.h API: `std::vector<int> linearSearchByCity(const std::vector<Driver>&, int city);`
       `Driver* binarySearchById(std::vector<Driver>& sortedDrivers, int id);` (NULL if absent;
        precondition sorted ascending by id -- initDrivers output already is).
- simulation.h API (FINAL):
    `void tick(std::vector<Driver>&, std::queue<SeatSlot>&, double& simTime, const Graph&);`
    `void departDriver(Driver&, const Graph&);`  (route only; NO payStack dependency)
    `void completeTrip(Driver&, std::queue<SeatSlot>&);`
    `Customer makeRandomCustomer();`  (id starts 1000, src=rand%DRIVERS+1, dest!=src)
    `void maybeSpawnRider(std::vector<Driver>&, std::queue<SeatSlot>&, double simTime, double& nextSpawnTime);`
  DECISION RESOLVED: fares are NOT computed in departDriver/tick. They are
  computed in MAIN at payment time via segmentFares on the user's driver. So main
  must SNAPSHOT the user's fare/passengers BEFORE completeTrip wipes the seats
  (completeTrip resets seats). Plan for main: when the user's driver flips to
  TRAVELLING, immediately compute segmentFares from driver.route + its filled
  seats (build vector<Customer> from seats), find the user's own fare by
  customerId, and pushPayments. Then user watches RIDE, then PAYMENT pops it.
- ui.h API: `enum Key { KEY_NONE, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_ENTER, KEY_QUIT, KEY_OTHER };`
    `void enableRawMode(); void disableRawMode(); Key readKey();` (readKey NON-BLOCKING,
     returns KEY_NONE if nothing pending -- so the main loop keeps animating)
    `void drawHome(const std::vector<Driver>&, double simTime);`
    `void drawMapSelect(int highlightedCity, const char* title);`
    `void drawSeatScreen(const Driver&, int mySeat);`
    `void drawRideSim(const Driver&);`
    `void drawPayment(double myFare);`
  All draw* are PURE. cityColor: 1..5 -> 31..35, City Circle(6) -> 37. ESC[?25l/h
  hides/shows cursor. clearScreen = ESC[2J ESC[H.
- NEXT (main.cpp, Step 11 -- FINAL): the state machine.
  STATES: HOME, MAP_SELECT_SRC, MAP_SELECT_DST, SEAT, RIDE, PAYMENT.
  Setup: srand(time(0)) ONCE; graph=buildGraph(); drivers=initDrivers() (or
  loadDrivers if file); seatQueue=initQueue(drivers); payStack empty; history=
  loadHistory(); simTime=0; nextSpawn=SPAWN_INTERVAL; enableRawMode(); atexit/
  cleanup disableRawMode.
  LOOP each iteration: maybeSpawnRider(...); tick(...); read key; per-state draw +
  transitions; small real-time sleep (~1 sec/tick via usleep, or faster for snappy
  demo e.g. 200ms and scale). NOTE TIME_SCALE=2.5 sim-min per REAL second, so to
  honour WAIT_TIMEOUT=4 real sec, loop should sleep ~1s per tick OR call tick once
  per ~1 real second. Plan: usleep(200000) (0.2s) and call tick every 5th iter, OR
  simpler usleep(1000000) 1s/tick. For snappy demo I'll sleep 250ms and tick each
  loop (faster clock) -- acceptable; document.
  BOOKING FLOW: HOME + ENTER -> MAP_SELECT_SRC (arrows over cities 1..5, skip 6) ->
  ENTER sets src -> MAP_SELECT_DST -> ENTER sets dst -> assignSeat(queue, myCustomer,
  drivers, simTime) -> remember myDriverId + mySeat (find which driver/seat got it:
  assignSeat doesn't return that -> WORKAROUND: scan drivers for seat with
  customerId==myCustomer.id right after). -> SEAT screen until that driver.status
  becomes TRAVELLING -> at that transition compute fare: build vector<Customer> from
  driver's filled seats, segmentFares(driver.route, graph, pax), find my fare by my
  customerId, pushPayments(payStack, {myCustomerId,myFare}); also snapshot myFare.
  -> RIDE screen until driver.status != TRAVELLING (completeTrip ran) -> PAYMENT
  screen; ENTER -> processNext(payStack); addToHistory(history, booking); back HOME.
  EDGE: if my car cancels (status EMPTY before TRAVELLING, e.g. timeout 0 riders --
  won't happen since I'm aboard) handle gracefully. On quit: saveDrivers, saveHistory,
  disableRawMode.
  WATCH OUT: completeTrip wipes seats, so fare MUST be computed at the SEAT->RIDE
  (departure) transition, before completion. Already planned above.
- main.cpp DONE (FINAL). States: ST_HOME, ST_MAP_SRC, ST_MAP_DST, ST_SEAT,
  ST_RIDE, ST_PAYMENT, ST_BOOK_FAIL. Loop: usleep 100ms, tick ~1/sec (TICKS_EVERY
  10), readKey non-blocking, redraw only on change. User booking via bookUserSeat()
  (matches the car AT the pickup city -- you board where the car is -- and removes
  that one slot from the queue). Fare computed in chargeUser() at the SEAT->RIDE
  transition via segmentFares, then pushPayments. Pay (ENTER) -> processNext +
  addToHistory -> HOME. srand(time(0)) called once at top of main.
- ui.cpp enableRawMode now also sets O_NONBLOCK on stdin (fcntl) so read() never
  stalls the sim even if stdin is a pipe (made the headless smoke test possible;
  harmless on a real TTY).
- DESIGN NOTE (viva): user is matched to the source-city driver (board where the
  car is) so passenger.source == route start == fare is correct; the global FIFO
  queue + assignSeat power the AMBIENT random riders (the queue-fairness story).

## Build command (whole project, once all files exist)
    g++ -std=c++11 *.cpp -o carpool
