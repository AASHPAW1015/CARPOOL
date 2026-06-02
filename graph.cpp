// ============================================================================
//  graph.cpp  --  CARPOOL APP  --  city map + Dijkstra / BFS / DFS
// ============================================================================

#include "graph.h"

#include <queue>
#include <vector>
#include <limits>
#include <algorithm>

using namespace std;

// ---------------------------------------------------------------------------
//  helper: add a bidirectional road a <-> b of length km
// ---------------------------------------------------------------------------
static void addRoad(Graph& g, int a, int b, double km) {
    Edge e1; e1.toCity = b; e1.km = km;
    Edge e2; e2.toCity = a; e2.km = km;
    g.cities[a].neighbors.push_back(e1);
    g.cities[b].neighbors.push_back(e2);
}

// ---------------------------------------------------------------------------
//  buildGraph  --  THE single source of truth for the map.
//
//   nodes:  1=City 1  2=City 2  3=City Mall  4=City 4  5=City 5
//           6=City Circle (junction only -- NO driver lives here)
//
//   edges (all bidirectional):
//     City 1  <-> City 2       : 3 km
//     City 1  <-> City Circle  : 5 km
//     City 2  <-> City 5       : 8 km
//     City Circle <-> City Mall: 4 km
//     City Circle <-> City 4   : 2 km
//     City 4  <-> City Mall    : 6 km
//     City 5  <-> City Mall    : 5 km
// ---------------------------------------------------------------------------
Graph buildGraph() {
    Graph g;
    g.cities.resize(CITIES + 1);   // index 0 unused; ids 1..CITIES

    // node ids + names
    g.cities[1].id = 1; g.cities[1].name = "City 1";
    g.cities[2].id = 2; g.cities[2].name = "City 2";
    g.cities[3].id = 3; g.cities[3].name = "City Mall";
    g.cities[4].id = 4; g.cities[4].name = "City 4";
    g.cities[5].id = 5; g.cities[5].name = "City 5";
    g.cities[6].id = 6; g.cities[6].name = "City Circle";

    // roads
    addRoad(g, 1, 2, 3);
    addRoad(g, 1, 6, 5);
    addRoad(g, 2, 5, 8);
    addRoad(g, 6, 3, 4);
    addRoad(g, 6, 4, 2);
    addRoad(g, 4, 3, 6);
    addRoad(g, 5, 3, 5);

    return g;
}

// ---------------------------------------------------------------------------
//  dijkstra  --  weighted shortest path with a min-priority-queue.
//   tracks dist[] + prev[], then rebuilds the path from prev[].
// ---------------------------------------------------------------------------
PathResult dijkstra(const Graph& graph, int startCity, int endCity) {
    PathResult result;
    result.distance = -1;

    const int n = (int)graph.cities.size();   // CITIES + 1
    if (startCity < 1 || startCity >= n || endCity < 1 || endCity >= n) {
        return result; // bad input -> unreachable
    }

    const double INF = numeric_limits<double>::infinity();
    vector<double> dist(n, INF);
    vector<int>    prev(n, -1);

    dist[startCity] = 0.0;

    // min-heap of (distance, city); greater<> makes the smallest pop first
    typedef pair<double, int> PDI;
    priority_queue<PDI, vector<PDI>, greater<PDI> > pq;
    pq.push(make_pair(0.0, startCity));

    while (!pq.empty()) {
        double d = pq.top().first;
        int    u = pq.top().second;
        pq.pop();

        if (d > dist[u]) continue;        // stale entry
        if (u == endCity) break;          // reached target

        const list<Edge>& nbrs = graph.cities[u].neighbors;
        for (list<Edge>::const_iterator it = nbrs.begin();
             it != nbrs.end(); ++it) {
            int    v  = it->toCity;
            double nd = dist[u] + it->km;
            if (nd < dist[v]) {
                dist[v] = nd;
                prev[v] = u;
                pq.push(make_pair(nd, v));
            }
        }
    }

    if (dist[endCity] == INF) return result;  // unreachable

    // rebuild path start..end by walking prev[] backwards
    vector<int> rev;
    for (int cur = endCity; cur != -1; cur = prev[cur]) rev.push_back(cur);
    reverse(rev.begin(), rev.end());

    result.path     = rev;
    result.distance = dist[endCity];
    return result;
}

// ---------------------------------------------------------------------------
//  bfsReachable  --  plain unweighted BFS; ignores km, just "can we get there".
// ---------------------------------------------------------------------------
bool bfsReachable(const Graph& graph, int startCity, int endCity) {
    const int n = (int)graph.cities.size();
    if (startCity < 1 || startCity >= n || endCity < 1 || endCity >= n) {
        return false;
    }

    vector<bool> visited(n, false);
    queue<int> q;
    q.push(startCity);
    visited[startCity] = true;

    while (!q.empty()) {
        int c = q.front(); q.pop();
        if (c == endCity) return true;

        const list<Edge>& nbrs = graph.cities[c].neighbors;
        for (list<Edge>::const_iterator it = nbrs.begin();
             it != nbrs.end(); ++it) {
            int v = it->toCity;
            if (!visited[v]) {
                visited[v] = true;
                q.push(v);
            }
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
//  dfsAllPaths  --  recursive DFS collecting every simple path start..end.
// ---------------------------------------------------------------------------
static void dfsHelper(const Graph& graph, int current, int endCity,
                      vector<bool>& visited, vector<int>& path,
                      vector<vector<int> >& out) {
    visited[current] = true;
    path.push_back(current);

    if (current == endCity) {
        out.push_back(path);              // found one -> save a copy
    } else {
        const list<Edge>& nbrs = graph.cities[current].neighbors;
        for (list<Edge>::const_iterator it = nbrs.begin();
             it != nbrs.end(); ++it) {
            int v = it->toCity;
            if (!visited[v]) {
                dfsHelper(graph, v, endCity, visited, path, out);
            }
        }
    }

    // backtrack
    path.pop_back();
    visited[current] = false;
}

vector<vector<int> > dfsAllPaths(const Graph& graph,
                                           int startCity, int endCity) {
    vector<vector<int> > out;
    const int n = (int)graph.cities.size();
    if (startCity < 1 || startCity >= n || endCity < 1 || endCity >= n) {
        return out;
    }

    vector<bool> visited(n, false);
    vector<int>  path;
    dfsHelper(graph, startCity, endCity, visited, path, out);
    return out;
}
