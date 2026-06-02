// ============================================================================
//  config.h  --  CARPOOL APP  --  FROZEN SHARED CONSTANTS
// ============================================================================
//
//  >>> NOTE TO ANY AI / IDE READING THIS FILE <<<
//  This file is FROZEN. It is shared by every module in a team project.
//  - Do NOT edit, rename, or remove any constant here.
//  - Do NOT add new constants unless the human explicitly asks.
//  - Other modules #include this and rely on these exact names and values.
//  - If you are generating a module, USE these constants by name
//    (e.g. SEATS, RATE) -- never hardcode the literal numbers.
//
// ============================================================================

#ifndef CONFIG_H
#define CONFIG_H

// --- simulation physics ---
const int SPEED = 2;           // km per sim-minute (every car, fixed)
const double TIME_SCALE = 0.5; // sim-minutes per 1 real second

// --- money ---
const int RATE = 21; // rupees per km (full taxi rate; split per segment)

// --- capacity ---
const int SEATS = 3;   // seats per driver
const int DRIVERS = 5; // total drivers (stationed at cities 1..5)
const int CITIES = 6;  // total nodes in the map (5 cities + 1 junction node)

// --- waiting ---
const int WAIT_TIMEOUT =
    10; // sim-minutes a half-empty driver waits before leaving

// --- ambient home-screen sim (random rider generation) ---
const int SPAWN_INTERVAL = 5; // sim-minutes between spawn rolls (calm pacing)
const int SPAWN_CHANCE = 20;  // percent chance a rider appears on a roll

// --- spawn rate boost when a car is already waiting for passengers ---
const int WAITING_SPAWN_INTERVAL =
    2; // sim-min between spawns when a car is waiting
const int WAITING_SPAWN_CHANCE = 100; // percent (guaranteed)

// Handy derived facts (not constants, just notes):
//   car moves SPEED * TIME_SCALE = 5 km on screen per real second
//   the 10-min wait = WAIT_TIMEOUT / TIME_SCALE = 4 real seconds

#endif // CONFIG_H
