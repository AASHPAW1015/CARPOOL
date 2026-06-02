// ============================================================================
//  routing.h  --  CARPOOL APP  --  multi-destination route + segment fares
// ============================================================================
//
//  multiRoute: a full car has up to 3 destinations. We chain Dijkstra greedily
//  (always head to the nearest not-yet-visited destination) to build one path
//  that covers them all, plus the total km.
//
//  segmentFares: THE pricing rule. Each road segment costs RATE*km; that cost
//  is split among ONLY the passengers actually aboard on that segment. Ride a
//  stretch alone -> pay full; share it -> split it. The driver always earns
//  exactly RATE * totalKm.
//
// ============================================================================

#ifndef ROUTING_H
#define ROUTING_H

#include <vector>
#include "types.h"    // Customer (frozen)
#include "config.h"   // RATE (frozen)
#include "graph.h"    // Graph, dijkstra, PathResult

using namespace std;

// result of covering several destinations in one chained route
struct MultiRouteResult {
    vector<int> fullPath;   // city ids in visiting order, starting at startCity
    double           totalKm;    // sum of all chained Dijkstra distances
};

// --- greedy nearest-neighbour chaining of Dijkstra over all destinations ---
MultiRouteResult multiRoute(const Graph& graph, int startCity,
                            const vector<int>& dests);

// --- per-passenger fare: split each segment's RATE*km among those aboard it ---
vector<double> segmentFares(const vector<int>& fullPath,
                                 const Graph& graph,
                                 const vector<Customer>& passengers);

// --- is this passenger in the car on the segment cityA -> cityB? ---
//     (boarded at/before A AND gets off at/after B, by position in fullPath)
bool isOnSegment(const Customer& passenger, int cityA, int cityB,
                 const vector<int>& fullPath);

#endif // ROUTING_H
