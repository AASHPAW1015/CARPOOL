// ============================================================================
//  ui.cpp  --  CARPOOL APP  --  UI REBUILD V2
// ============================================================================
//  ANSI + termios only. No ncurses. Two input modes:
//    1. TYPED (cooked): readLine() — getline, trim, lowercase
//    2. RAW: enableRawMode/disableRawMode/readKey — arrow keys for dest select
//  All draw* are PURE (read + print, never mutate).
// ============================================================================

#include "ui.h"
#include "seatqueue.h"       // countFilled (pure read)
#include "leaderboard.h"     // BSTNode, insertDriver, inorder, freeTree
#include "sorting.h"         // compareTimings

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <vector>

using namespace std;

// ===========================================================================
//  the hardcoded ASCII map
// ===========================================================================
static const char* MAP_LINES[] = {
  "        +--------------+              5km                    ",
  "        |    CITY 5    |-----------------------------+       ",
  "        +--------------+                             |       ",
  "               |                                     |       ",
  "               |                                     |       ",
  "           8km |                                     |       ",
  "               |                             +-----------+         6km",
  "               +-------+                     | CITY MALL |---------+  ",
  "                       |                     +-----------+          |  ",
  "                       |                          |                 |  ",
  "        +--------------+                          |          +----------+",
  "        |    CITY 2    |                       4km|          |  CITY 4  |",
  "        +--------------+                          |          +----------+",
  "               |                                  |            /       ",
  "           3km |                               ~----~     2km        ",
  "               |                              / city \\   /           ",
  "        +--------------+         5km         | circle |_/            ",
  "        |    CITY 1    |---------------------\\_______/               ",
  "        +--------------+                                               ",
  nullptr
};

// ===========================================================================
//  terminal width detection + centering
// ===========================================================================
static int getTermWidth() {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0)
        return w.ws_col;
    return 80;
}

static string getIndent() {
    int termCols = getTermWidth();
    int pad = (termCols - 70) / 2;
    if (pad < 0) pad = 0;
    return string(pad, ' ');
}

// ===========================================================================
//  ANSI helpers
// ===========================================================================
static void clearScreen(int screenId) {
    static int lastScreenId = -1;
    static int lastCols = 0;
    int cols = getTermWidth();

    if (lastScreenId != screenId || lastCols != cols) {
        printf("\033[2J\033[H"); // Full clear on screen change or resize
        lastScreenId = screenId;
        lastCols = cols;
    } else {
        printf("\033[H");      // Just home cursor for smooth updates
    }
}

// fixed colour per city: 1→31(red) 2→32(green) 3→33(yellow) 4→34(blue) 5→35(magenta) 6→37(white)
static int cityColor(int id) {
    if (id >= 1 && id <= 5) return 30 + id;
    return 37;
}

static const char* cityNamePlain(int id) {
    switch (id) {
        case 1: return "City 1";
        case 2: return "City 2";
        case 3: return "City Mall";
        case 4: return "City 4";
        case 5: return "City 5";
        case 6: return "City Circle";
    }
    return "?";
}

static string col(int code, const string& s) {
    ostringstream o;
    o << "\033[" << code << "m" << s << "\033[0m";
    return o.str();
}

static string colBright(int code, const string& s) {
    ostringstream o;
    o << "\033[1;" << code << "m" << s << "\033[0m";
    return o.str();
}

// ===========================================================================
//  PUBLIC helpers: carTag and cityColName
// ===========================================================================
string carTag(int driverId) {
    ostringstream o;
    o << "\033[" << (30 + driverId) << "m[C" << driverId << "]\033[0m";
    return o.str();
}

string cityColName(int cityId) {
    return col(cityColor(cityId), cityNamePlain(cityId));
}

// ===========================================================================
//  raw mode
// ===========================================================================
static struct termios g_origTermios;
static int            g_origFlags = 0;
static bool           g_rawActive = false;

void enableRawMode() {
    tcgetattr(STDIN_FILENO, &g_origTermios);
    struct termios raw = g_origTermios;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN]  = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    g_origFlags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, g_origFlags | O_NONBLOCK);

    printf("\033[?25l");   // hide cursor
    fflush(stdout);
    g_rawActive = true;
}

void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_origTermios);
    fcntl(STDIN_FILENO, F_SETFL, g_origFlags);
    printf("\033[?25h");   // show cursor
    printf("\033[0m");     // reset colours
    fflush(stdout);
    g_rawActive = false;
}

// ===========================================================================
//  readKey  --  non-blocking, arrow sequences: ESC [ A/B/C/D
// ===========================================================================
Key readKey() {
    char c;
    if (read(STDIN_FILENO, &c, 1) <= 0) return KEY_NONE;
    if (c == '\n' || c == '\r') return KEY_ENTER;
    if (c == 'q'  || c == 'Q')  return KEY_QUIT;
    if (c == 27) {
        char seq0, seq1;
        if (read(STDIN_FILENO, &seq0, 1) <= 0) return KEY_OTHER;
        if (read(STDIN_FILENO, &seq1, 1) <= 0) return KEY_OTHER;
        if (seq0 == '[') {
            switch (seq1) {
                case 'A': return KEY_UP;
                case 'B': return KEY_DOWN;
                case 'C': return KEY_RIGHT;
                case 'D': return KEY_LEFT;
            }
        }
        return KEY_OTHER;
    }
    return KEY_OTHER;
}

// ===========================================================================
//  readLine  --  COOKED mode, blocks until enter
// ===========================================================================
string readLine() {
    if (g_rawActive) disableRawMode();

    string line;
    getline(cin, line);

    // trim
    size_t start = line.find_first_not_of(" \t\r\n");
    size_t end   = line.find_last_not_of(" \t\r\n");
    if (start == string::npos) return "";
    line = line.substr(start, end - start + 1);

    // lowercase
    for (size_t i = 0; i < line.size(); ++i)
        line[i] = tolower(line[i]);

    return line;
}

// ===========================================================================
//  renderMapLine  --  colour city names in a map line, optional highlight
// ===========================================================================
struct CityToken {
    const char* search;
    int         colorCode;
    int         cityId;
};

static const CityToken TOKENS[] = {
    { "CITY MALL",   33, 3 },
    { "CITY 5",      35, 5 },
    { "CITY 2",      32, 2 },
    { "CITY 4",      34, 4 },
    { "CITY 1",      31, 1 },
    { "city circle",  0, 6 },
    { "circle",       0, 6 },
    { nullptr,        0, 0 }
};

// Box positions: which line index holds which city's top/mid/bottom box cell.
// Used so the highlighted city's entire +---+ / | name | segment is coloured.
struct BoxInfo { int lineIdx; int cityId; size_t pos; size_t len; };
static const BoxInfo BOXES[] = {
    // City 5 (magenta)
    { 0, 5,  8, 16 }, { 1, 5,  8, 16 }, { 2, 5,  8, 16 },
    // City Mall (yellow)
    { 6, 3, 45, 13 }, { 7, 3, 45, 13 }, { 8, 3, 45, 13 },
    // City 2 (green)  -- lines 10-12 also have City 4 at a different offset
    {10, 2,  8, 16 }, {11, 2,  8, 16 }, {12, 2,  8, 16 },
    // City 4 (blue)
    {10, 4, 61, 12 }, {11, 4, 61, 12 }, {12, 4, 61, 12 },
    // City 1 (red)
    {16, 1,  8, 16 }, {17, 1,  8, 16 }, {18, 1,  8, 16 },
    { -1, 0,  0,  0 }  // sentinel
};

// mode: 0 = normal colour, >0 = highlight that city id
// routeSet: if non-null, cities in this set get bright cyan instead of normal
static string renderMapLine(const char* line, int lineIdx, int highlight,
                                 const vector<int>* routeSet = nullptr) {
    string s(line);
    string result;

    // find the highlighted box region for this line (if any)
    int    boxStart = -1, boxLen = 0, boxColor = 0;
    if (highlight > 0) {
        for (int b = 0; BOXES[b].lineIdx >= 0; ++b) {
            if (BOXES[b].lineIdx == lineIdx && BOXES[b].cityId == highlight) {
                boxStart = (int)BOXES[b].pos;
                boxLen   = (int)BOXES[b].len;
                boxColor = cityColor(highlight);
                break;
            }
        }
    }

    size_t pos = 0;
    while (pos < s.size()) {
        // emit the entire highlighted box cell as one coloured block
        if (boxStart >= 0 && (int)pos == boxStart) {
            ostringstream o;
            o << "\033[" << boxColor << "m"
              << s.substr(boxStart, boxLen)
              << "\033[0m";
            result += o.str();
            pos += (size_t)boxLen;
            continue;
        }

        bool matched = false;
        for (int t = 0; TOKENS[t].search != nullptr; ++t) {
            size_t len = strlen(TOKENS[t].search);
            if (s.compare(pos, len, TOKENS[t].search) == 0) {
                int cid = TOKENS[t].cityId;
                string name(TOKENS[t].search);

                if (routeSet) {
                    bool inRoute = false;
                    for (size_t ri = 0; ri < routeSet->size(); ++ri) {
                        if ((*routeSet)[ri] == cid) { inRoute = true; break; }
                    }
                    if (inRoute) {
                        result += colBright(36, name);  // bright cyan
                    } else if (cid == 6) {
                        result += col(37, name);
                    } else {
                        ostringstream o;
                        o << "\033[" << TOKENS[t].colorCode << "m" << name << "\033[0m";
                        result += o.str();
                    }
                } else if (cid == 6) {
                    result += "\033[37m" + name + "\033[0m";
                } else {
                    ostringstream o;
                    o << "\033[" << TOKENS[t].colorCode << "m" << name << "\033[0m";
                    result += o.str();
                }
                pos += len;
                matched = true;
                break;
            }
        }
        if (!matched) {
            result += s[pos];
            ++pos;
        }
    }
    return result;
}

// ===========================================================================
//  printMap  --  render full map + DRIVERS panel (used by drawHome)
// ===========================================================================
void printMap(const Driver drivers[], double /*simTime*/) {
    string indent = getIndent();

    for (int i = 0; MAP_LINES[i] != nullptr; ++i) {
        printf("%s%s\n", indent.c_str(), renderMapLine(MAP_LINES[i], i, 0).c_str());
    }

    printf("\n");
    printf("%s\033[1mDRIVERS:\033[0m\n", indent.c_str());

    for (int i = 0; i < DRIVERS; ++i) {
        const Driver& d = drivers[i];
        int filled = countFilled(d);

        // seat dots
        string dots;
        for (int s = 0; s < SEATS; ++s) {
            if (d.seats[s].filled) {
                ostringstream o;
                o << "\033[" << (30 + d.id) << "m\xe2\x97\x8f\033[0m";  // ●
                dots += o.str();
            } else {
                dots += "\033[2m\xe2\x97\x8b\033[0m";  // ○
            }
        }

        string tag = carTag(d.id);
        string city = cityColName(d.currentCity);
        const char* statusStr = "idle";
        if (d.status == WAITING)    statusStr = "waiting";
        if (d.status == TRAVELLING) statusStr = "travelling";

        printf("%s  %s  %-11s @ %s   seats[%s] (%d/%d)\n",
               indent.c_str(), tag.c_str(), statusStr, city.c_str(),
               dots.c_str(), filled, SEATS);

        if (d.status == TRAVELLING && !d.route.empty()) {
            vector<int> r(d.route.begin(), d.route.end());
            if (r.size() >= 2) {
                double frac = (d.totalKm > 0) ? (d.travelledKm / d.totalKm) : 1.0;
                if (frac < 0) frac = 0;
                if (frac > 1) frac = 1;
                int segments = (int)r.size() - 1;
                int segIdx = (int)(frac * segments);
                if (segIdx >= segments) segIdx = segments - 1;
                if (segIdx < 0) segIdx = 0;
                printf("%s       >> %s -> %s  (%.0f/%.0f km)\n",
                       indent.c_str(),
                       cityColName(r[segIdx]).c_str(),
                       cityColName(r[segIdx + 1]).c_str(),
                       d.travelledKm, d.totalKm);
            }
        }
    }
}

// ===========================================================================
//  drawHomeUpdate  --  Update HOME screen map without touching prompt
// ===========================================================================
void drawHomeUpdate(const Driver drivers[], double simTime) {
    printf("\033[s"); // Save cursor
    printf("\033[H"); // Move to top-left

    string indent = getIndent();
    printf("%s\033[1m================ CARPOOL ================\033[0m   sim-time: %.0f min\n\n",
           indent.c_str(), simTime);

    printMap(drivers, simTime);

    printf("\033[u"); // Restore cursor
    fflush(stdout);
}

// ===========================================================================
//  drawHome  --  SCREEN 1
// ===========================================================================
void drawHome(const Driver drivers[], double simTime) {
    clearScreen(1);
    string indent = getIndent();

    printf("%s\033[1m================ CARPOOL ================\033[0m   sim-time: %.0f min\n\n",
           indent.c_str(), simTime);

    printMap(drivers, simTime);

    printf("\n");
    printf("%s  \033[2mtype a city name (city 1 / city 2 / city mall / city 4 / city 5)\033[0m\n", indent.c_str());
    printf("%s  \033[2mtype \"leaderboard\" for rankings   |   type \"q\" to quit\033[0m\n\n", indent.c_str());
    printf("%s  > Where are you? : ", indent.c_str());
    printf("\033[J"); // clear remainder of screen
    fflush(stdout);
}

// ===========================================================================
//  drawMapSelect  --  SCREEN 2 (dest select with highlight)
// ===========================================================================
void drawMapSelect(int highlightedCity, int sourceCity) {
    clearScreen(2);
    string indent = getIndent();

    printf("%s\033[1m== SELECT DESTINATION ==\033[0m\n", indent.c_str());
    printf("%s(from: %s)\n\n", indent.c_str(), cityColName(sourceCity).c_str());

    for (int i = 0; MAP_LINES[i] != nullptr; ++i) {
        printf("%s%s\n", indent.c_str(), renderMapLine(MAP_LINES[i], i, highlightedCity).c_str());
    }

    printf("\n");
    printf("%s  selected: %s\n", indent.c_str(), cityColName(highlightedCity).c_str());
    printf("%s  \033[2m(City Circle is a junction \xe2\x80\x94 not selectable)\033[0m\n", indent.c_str());
    printf("\n%s  [\xe2\x86\x91\xe2\x86\x93\xe2\x86\x90\xe2\x86\x92] move   [ENTER] confirm   [q] cancel\n", indent.c_str());
    printf("\033[J"); // clear remainder of screen
    fflush(stdout);
}

static int visibleLength(const string& str) {
    int len = 0;
    bool inAnsi = false;
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '\033') inAnsi = true;
        else if (inAnsi && str[i] == 'm') inAnsi = false;
        else if (!inAnsi) {
            if ((str[i] & 0xC0) != 0x80) len++;
        }
    }
    return len;
}

static void printBoxLine(const string& indent, const string& content) {
    int visLen = visibleLength(content);
    int pad = 36 - visLen;
    if (pad < 0) pad = 0;
    printf("%s  |%s%*s|\n", indent.c_str(), content.c_str(), pad, "");
}

// ===========================================================================
//  drawSeatScreen  --  SCREEN 3 (seat wait / your ride)
// ===========================================================================
void drawSeatScreen(const Driver& driver, int mySeat, double simTime) {
    clearScreen(3);
    string indent = getIndent();

    printf("%s\033[1m== YOUR RIDE ==\033[0m\n\n", indent.c_str());

    // time remaining
    double remaining = WAIT_TIMEOUT - (simTime - driver.waitStartTime);
    int realSec = (int)ceil(remaining / TIME_SCALE);
    if (realSec < 0) realSec = 0;
    int filled = countFilled(driver);
    bool departing = (filled >= SEATS || driver.status == TRAVELLING);

    string timeStr;
    if (departing) {
        timeStr = "\033[1;32mdeparting now!\033[0m";
    } else {
        ostringstream o;
        o << "time left: " << realSec << "s";
        timeStr = o.str();
    }

    // car rectangle
    printf("%s  +------------------------------------+\n", indent.c_str());
    
    string header = "  ( DRV " + carTag(driver.id) + " )";
    int padMid = 36 - visibleLength(header) - visibleLength(timeStr) - 2;
    if (padMid < 0) padMid = 0;
    string space(padMid, ' ');
    printBoxLine(indent, header + space + timeStr + "  ");
    printBoxLine(indent, " "); // empty line

    for (int s = 0; s < SEATS; ++s) {
        const Seat& seat = driver.seats[s];
        ostringstream line;

        if (s == mySeat && seat.filled) {
            line << "  seat " << (s + 1) << ": \033[1;32m[YOU]\033[0m  "
                 << cityColName(seat.passengerSource)
                 << " -> " << cityColName(seat.passengerDest);
        } else if (seat.filled) {
            line << "  seat " << (s + 1) << ": rider#" << seat.customerId << "  "
                 << cityColName(seat.passengerSource)
                 << " -> " << cityColName(seat.passengerDest);
        } else {
            line << "  seat " << (s + 1) << ": \033[2mempty -- waiting...\033[0m";
        }

        printBoxLine(indent, line.str());

        if (s < SEATS - 1)
            printBoxLine(indent, " ");
    }

    printf("%s  +------------------------------------+\n", indent.c_str());
    printf("%s  %d/%d seats filled.\n\n", indent.c_str(), filled, SEATS);
    printf("\033[J"); // clear remainder of screen

    fflush(stdout);
}

// ===========================================================================
//  helper: edgeKm — get the km between two adjacent cities from the graph
// ===========================================================================
static double edgeKm(const Graph& g, int a, int b) {
    if (a < 1 || a >= (int)g.cities.size()) return 0.0;
    const list<Edge>& nbrs = g.cities[a].neighbors;
    for (list<Edge>::const_iterator it = nbrs.begin();
         it != nbrs.end(); ++it) {
        if (it->toCity == b) return it->km;
    }
    return 0.0;
}

// ===========================================================================
//  drawRideSim  --  SCREEN 4 (trip in progress)
// ===========================================================================
void drawRideSim(const Driver& driver, const Graph& graph) {
    clearScreen(4);
    string indent = getIndent();

    printf("%s\033[1m== TRIP IN PROGRESS ==\033[0m\n\n", indent.c_str());

    vector<int> r(driver.route.begin(), driver.route.end());

    // print map with route cities highlighted in bright cyan
    for (int i = 0; MAP_LINES[i] != nullptr; ++i) {
        printf("%s%s\n", indent.c_str(), renderMapLine(MAP_LINES[i], i, 0, &r).c_str());
    }
    printf("\n");

    // find current segment using edgeKm
    int currentFromIdx = 0;
    if (r.size() >= 2) {
        double walked = 0;
        for (size_t i = 0; i + 1 < r.size(); ++i) {
            double seg = edgeKm(graph, r[i], r[i + 1]);
            if (driver.travelledKm < walked + seg) {
                currentFromIdx = (int)i;
                break;
            }
            walked += seg;
            currentFromIdx = (int)i;
        }
        if (driver.travelledKm >= driver.totalKm && driver.totalKm > 0)
            currentFromIdx = (int)r.size() - 2;
    }

    // route display: visited / current / upcoming
    printf("%s  route:  ", indent.c_str());
    for (size_t i = 0; i < r.size(); ++i) {
        string name = cityNamePlain(r[i]);
        if ((int)i <= currentFromIdx) {
            // visited or current-from (dim)
            printf("\033[2m[%s]\033[0m", name.c_str());
        } else if ((int)i == currentFromIdx + 1) {
            // current-to (bright cyan)
            printf("\033[1;36m[%s]\033[0m", name.c_str());
        } else {
            // upcoming (normal)
            printf("[%s]", name.c_str());
        }

        if (i + 1 < r.size()) {
            if ((int)i < currentFromIdx) {
                printf(" \033[2m\xe2\x94\x80\xe2\x94\x80\xe2\x9c\x93\xe2\x94\x80\xe2\x94\x80>\033[0m ");   // ──✓──>
            } else if ((int)i == currentFromIdx) {
                printf(" \033[1;36m\xe2\x94\x80\xe2\x94\x80\xe2\x97\x8f\xe2\x94\x80\xe2\x94\x80>\033[0m ");  // ──●──>
            } else {
                printf(" \xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80 ");   // ────
            }
        }
    }
    printf("\n\n");

    // progress bar (24 chars wide)
    double frac = (driver.totalKm > 0) ? (driver.travelledKm / driver.totalKm) : 1.0;
    if (frac < 0) frac = 0;
    if (frac > 1) frac = 1;
    int barW = 24;
    int fillCount = (int)(frac * barW);

    printf("%s  ", indent.c_str());
    for (int b = 0; b < barW; ++b) {
        if (b < fillCount) printf("\xe2\x96\x88");   // █
        else               printf("\xe2\x96\x91");   // ░
    }
    printf("   %.1f / %.1f km", driver.travelledKm, driver.totalKm);

    double remainingKm = driver.totalKm - driver.travelledKm;
    if (remainingKm < 0) remainingKm = 0;
    double remainingMin = (SPEED > 0) ? (remainingKm / SPEED) : 0;
    printf("   (~%.0f sim-min left)\n\n", remainingMin);

    if (driver.travelledKm >= driver.totalKm)
        printf("%s  \033[1;32mArrived!\033[0m\n", indent.c_str());

    printf("\033[J"); // clear remainder of screen
    fflush(stdout);
}

// ===========================================================================
//  drawPayment  --  SCREEN 5
// ===========================================================================
void drawPayment(double myFare, int driverId) {
    clearScreen(5);
    string indent = getIndent();

    // compute fare string and padding for box alignment
    ostringstream fs;
    fs << (int)myFare;
    string fareStr = fs.str();
    // "         <fare> rupees" should be centered in the 30-char box interior
    // box interior is 30 chars wide (between ║ chars)
    string fareLabel = fareStr + " rupees";
    int fareLen = (int)fareLabel.size();
    int farePadLeft = (30 - fareLen) / 2;
    int farePadRight = 30 - fareLen - farePadLeft;

    string drvLabel = "pay driver C" + to_string(driverId);
    int drvLen = (int)drvLabel.size();
    int drvPadLeft = (30 - drvLen) / 2;
    int drvPadRight = 30 - drvLen - drvPadLeft;

    printf("\n");
    printf("%s  \xe2\x95\x94", indent.c_str());
    for (int i = 0; i < 30; ++i) printf("\xe2\x95\x90");
    printf("\xe2\x95\x97\n");

    printf("%s  \xe2\x95\x91%*s%*s\xe2\x95\x91\n", indent.c_str(), 30, "", 0, "");

    printf("%s  \xe2\x95\x91%*s%s %s%*s\xe2\x95\x91\n", indent.c_str(),
           drvPadLeft, "", "pay driver", carTag(driverId).c_str(), drvPadRight - 2, "");

    printf("%s  \xe2\x95\x91%*s%*s\xe2\x95\x91\n", indent.c_str(), 30, "", 0, "");

    printf("%s  \xe2\x95\x91%*s\033[1;33m%s\033[0m%*s\xe2\x95\x91\n", indent.c_str(),
           farePadLeft, "", fareLabel.c_str(), farePadRight, "");

    printf("%s  \xe2\x95\x91%*s%*s\xe2\x95\x91\n", indent.c_str(), 30, "", 0, "");

    printf("%s  \xe2\x95\x91    [ P - PAY ]  [ S - SKIP ] \xe2\x95\x91\n", indent.c_str());

    printf("%s  \xe2\x95\x91%*s%*s\xe2\x95\x91\n", indent.c_str(), 30, "", 0, "");

    printf("%s  \xe2\x95\x9a", indent.c_str());
    for (int i = 0; i < 30; ++i) printf("\xe2\x95\x90");
    printf("\xe2\x95\x9d\n");

    printf("\n");
    printf("%s  press P to pay, S to skip\n", indent.c_str());
    printf("\033[J"); // clear remainder of screen
    fflush(stdout);
}

// ===========================================================================
//  drawLeaderboard  --  SCREEN 6
// ===========================================================================
void drawLeaderboard(const Driver drivers[]) {
    clearScreen(6);
    string indent = getIndent();

    // build BST fresh
    BSTNode* root = nullptr;
    for (int i = 0; i < DRIVERS; ++i) {
        root = insertDriver(root, drivers[i]);
    }

    // inorder → ascending; reverse → descending (most rides first)
    vector<Driver> sorted = inorder(root);
    reverse(sorted.begin(), sorted.end());
    freeTree(root);

    printf("\n");
    printf("%s  \xe2\x95\x94", indent.c_str());
    for (int i = 0; i < 42; ++i) printf("\xe2\x95\x90");
    printf("\xe2\x95\x97\n");
    printf("%s  \xe2\x95\x91           DRIVER LEADERBOARD             \xe2\x95\x91\n", indent.c_str());
    printf("%s  \xe2\x95\x9a", indent.c_str());
    for (int i = 0; i < 42; ++i) printf("\xe2\x95\x90");
    printf("\xe2\x95\x9d\n");
    printf("\n");

    printf("%s  rank   driver   home city         rides today\n", indent.c_str());
    printf("%s  \xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80   \xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80   \xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80         \xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\n", indent.c_str());

    for (size_t i = 0; i < sorted.size(); ++i) {
        const Driver& d = sorted[i];
        printf("%s   #%d    %s     %-18s    %d\n",
               indent.c_str(), (int)(i + 1),
               carTag(d.id).c_str(),
               cityColName(d.homeCity).c_str(),
               d.ridesServedToday);
    }

    printf("\n");

    // sorting comparison table
    compareTimings(drivers);

    printf("\n");
    printf("%s  press any key to return \xe2\x86\x92\n", indent.c_str());
    printf("\033[J"); // clear remainder of screen
    fflush(stdout);
}
