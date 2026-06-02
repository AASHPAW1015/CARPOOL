// ============================================================================
//  routing.cpp  --  CARPOOL APP  --  multi-destination route + segment fares
// ============================================================================

#include "routing.h"

#include <list>

using namespace std;

// ---------------------------------------------------------------------------
//  helper: km of the direct edge a <-> b (0 if a,b are not adjacent)
// ---------------------------------------------------------------------------
static double edgeKm(const Graph& g, int a, int b) {
    if (a < 1 || a >= (int)g.cities.size()) return 0.0;
    const list<Edge>& nbrs = g.cities[a].neighbors;
    for (list<Edge>::const_iterator it = nbrs.begin();
         it != nbrs.end(); ++it) {
        if (it->toCity == b) return it->km;
    }
    return 0.0;
}

// ---------------------------------------------------------------------------
//  helper: first index of city in path (-1 if absent)
// ---------------------------------------------------------------------------
static int indexOf(const vector<int>& path, int city) {
    for (size_t i = 0; i < path.size(); ++i) {
        if (path[i] == city) return (int)i;
    }
    return -1;
}

// ---------------------------------------------------------------------------
//  multiRoute  --  greedy nearest-neighbour, chaining Dijkstra.
//   start at startCity; repeatedly hop to the nearest remaining destination,
//   appending its path (minus the duplicated current city) until all covered.
// ---------------------------------------------------------------------------
MultiRouteResult multiRoute(const Graph& graph, int startCity,
                            const vector<int>& dests) {
    MultiRouteResult res;
    res.totalKm = 0.0;
    res.fullPath.push_back(startCity);

    int current = startCity;
    vector<int> remaining = dests;

    while (!remaining.empty()) {
        // of all remaining destinations, find the nearest by Dijkstra distance
        int        bestIdx  = -1;
        double     bestDist = -1;
        PathResult bestR;
        for (size_t i = 0; i < remaining.size(); ++i) {
            PathResult r = dijkstra(graph, current, remaining[i]);
            if (r.distance < 0) continue;                 // unreachable, skip
            if (bestIdx == -1 || r.distance < bestDist) {
                bestIdx  = (int)i;
                bestDist = r.distance;
                bestR    = r;
            }
        }
        if (bestIdx == -1) break;   // nothing reachable left -> stop

        // append the hop's path, skipping its first node (== current, a dup)
        for (size_t k = 1; k < bestR.path.size(); ++k) {
            res.fullPath.push_back(bestR.path[k]);
        }
        res.totalKm += bestR.distance;

        current = remaining[bestIdx];
        remaining.erase(remaining.begin() + bestIdx);
    }

    return res;
}

// ---------------------------------------------------------------------------
//  isOnSegment  --  passenger aboard on segment cityA -> cityB?
//   boarded at/before A AND gets off at/after B, measured by index in fullPath.
// ---------------------------------------------------------------------------
bool isOnSegment(const Customer& passenger, int cityA, int cityB,
                 const vector<int>& fullPath) {
    int idxA    = indexOf(fullPath, cityA);
    int idxB    = indexOf(fullPath, cityB);
    int srcIdx  = indexOf(fullPath, passenger.sourceCity);
    int destIdx = indexOf(fullPath, passenger.destCity);

    if (idxA < 0 || idxB < 0 || srcIdx < 0 || destIdx < 0) return false;
    return srcIdx <= idxA && destIdx >= idxB;
}

// ---------------------------------------------------------------------------
//  segmentFares  --  THE pricing. Per segment, split RATE*km by who's aboard.
//   driver always nets RATE*totalKm because every segment's full cost is
//   distributed (share * count == RATE*segKm).
// ---------------------------------------------------------------------------
vector<double> segmentFares(const vector<int>& fullPath,
                                 const Graph& graph,
                                 const vector<Customer>& passengers) {
    vector<double> fares(passengers.size(), 0.0);

    for (size_t i = 0; i + 1 < fullPath.size(); ++i) {
        int    A     = fullPath[i];
        int    B     = fullPath[i + 1];
        double segKm = edgeKm(graph, A, B);

        // who is in the car on this segment?
        vector<size_t> aboard;
        for (size_t p = 0; p < passengers.size(); ++p) {
            if (isOnSegment(passengers[p], A, B, fullPath)) {
                aboard.push_back(p);
            }
        }

        if (!aboard.empty()) {
            double share = (RATE * segKm) / (double)aboard.size();
            for (size_t k = 0; k < aboard.size(); ++k) {
                fares[aboard[k]] += share;
            }
        }
    }

    return fares;
}
