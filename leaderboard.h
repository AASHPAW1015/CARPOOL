// ============================================================================
//  leaderboard.h  --  CARPOOL APP  --  BST of drivers by rides served
// ============================================================================
//
//  The "tree" requirement. A binary search tree keyed on
//  Driver.ridesServedToday. inorder() walks it depth-first -> drivers ranked
//  ascending by rides (DFS). levelOrder() walks it breadth-first row by row
//  using a queue (BFS). searchDriver() finds a node in O(height).
//
//  This module is an island: it only needs types.h (Driver).
//
// ============================================================================

#ifndef LEADERBOARD_H
#define LEADERBOARD_H

#include <vector>
#include "types.h"   // Driver (frozen)

using namespace std;

// one node of the leaderboard BST
struct BSTNode {
    Driver   driver;
    BSTNode* left;
    BSTNode* right;
};

// --- BST insert, key = driver.ridesServedToday; returns the (new) root ---
BSTNode* insertDriver(BSTNode* root, const Driver& driver);

// --- inorder (DFS) traversal -> drivers ascending by ridesServedToday ---
vector<Driver> inorder(BSTNode* root);

// --- level-order (BFS) traversal -> root first, then each level down ---
vector<Driver> levelOrder(BSTNode* root);

// --- BST search by key (ridesServedToday); returns pointer or NULL ---
Driver* searchDriver(BSTNode* root, int key);

// --- free the whole tree (housekeeping) ---
void freeTree(BSTNode* root);

#endif // LEADERBOARD_H
