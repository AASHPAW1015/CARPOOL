// ============================================================================
//  graph.h  --  CARPOOL APP  --  city map + Dijkstra / BFS / DFS
// ============================================================================
//
//  The city network lives here. buildGraph() hardcodes the agreed map
//  (5 cities 1..5 with drivers + node 6 "City Circle", a junction-only
//  waypoint). All edges are bidirectional. Dijkstra gives weighted shortest
//  path; BFS just answers "is it reachable"; DFS lists every simple path.
//
// ============================================================================

#ifndef GRAPH_H
#define GRAPH_H

#include <vector>
#include "types.h"   // City, Edge (frozen)
#include "config.h"  // CITIES (frozen)

using namespace std;

// the whole map: cities indexed by id (cities[1]..cities[CITIES]).
// index 0 is an unused placeholder so id == vector index.
struct Graph {
    vector<City> cities;   // size CITIES + 1
};

// result of a single shortest-path query
struct PathResult {
    vector<int> path;      // city ids start..end (empty if unreachable)
    double           distance;  // total km (-1 if unreachable)
};

// --- build the fixed 6-node map (5 cities + City Circle junction) ---
Graph buildGraph();

// --- weighted shortest path (min-priority-queue Dijkstra) ---
PathResult dijkstra(const Graph& graph, int startCity, int endCity);

// --- plain BFS: can we even get from start to end? ---
bool bfsReachable(const Graph& graph, int startCity, int endCity);

// --- plain DFS: every simple path from start to end ---
vector<vector<int> > dfsAllPaths(const Graph& graph,
                                           int startCity, int endCity);

#endif // GRAPH_H
