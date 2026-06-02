// ============================================================================
//  ui.h  --  CARPOOL APP  --  UI REBUILD V2
// ============================================================================
//
//  Two input modes:
//    1. TYPED (cooked): readLine() for home screen + leaderboard trigger
//    2. RAW ARROW KEYS: enableRawMode/disableRawMode/readKey for dest select
//
//  All draw* functions are PURE (read + print only, never mutate state).
//
// ============================================================================

#ifndef UI_H
#define UI_H

#include <vector>
#include <string>
#include <queue>
#include <stack>
#include <list>
#include "types.h"       // Driver, Booking, Payment (frozen)
#include "graph.h"       // Graph

using namespace std;

// keys readKey() can report (KEY_NONE = nothing pending, non-blocking in raw mode)
enum Key {
    KEY_NONE,
    KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT,
    KEY_ENTER, KEY_QUIT, KEY_OTHER
};

// --- terminal raw mode (no line buffering, no echo, non-blocking reads) ---
void enableRawMode();
void disableRawMode();

// --- read one key event (raw mode); returns KEY_NONE if nothing pending ---
Key readKey();

// --- typed input (cooked mode): reads a full line, trims, lowercases ---
string readLine();

// --- coloured helpers ---
string carTag(int driverId);         // e.g. \033[31m[C1]\033[0m
string cityColName(int cityId);      // e.g. \033[31mCity 1\033[0m

// --- the big ASCII map with driver positions ---
void printMap(const Driver drivers[], double simTime);

// --- screens (all PURE: read + print only) ---
void drawHomeUpdate(const Driver drivers[], double simTime);
void drawHome(const Driver drivers[], double simTime);
void drawMapSelect(int highlightedCity, int sourceCity);
void drawSeatScreen(const Driver& driver, int mySeat, double simTime);
void drawRideSim(const Driver& driver, const Graph& graph);
void drawPayment(double myFare, int driverId);
void drawLeaderboard(const Driver drivers[]);

#endif // UI_H
