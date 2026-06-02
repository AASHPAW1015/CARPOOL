// ============================================================================
//  leaderboard.cpp  --  CARPOOL APP  --  BST of drivers by rides served
// ============================================================================

#include "leaderboard.h"

#include <queue>

using namespace std;

// ---------------------------------------------------------------------------
//  insertDriver  --  standard BST insert, key = ridesServedToday.
//   ties (== key) go to the right subtree, so duplicates are kept.
// ---------------------------------------------------------------------------
BSTNode* insertDriver(BSTNode* root, const Driver& driver) {
    if (root == NULL) {
        BSTNode* node = new BSTNode;
        node->driver = driver;
        node->left   = NULL;
        node->right  = NULL;
        return node;
    }

    if (driver.ridesServedToday < root->driver.ridesServedToday) {
        root->left = insertDriver(root->left, driver);
    } else {
        root->right = insertDriver(root->right, driver);
    }
    return root;
}

// ---------------------------------------------------------------------------
//  inorder  --  DFS: left, node, right -> ascending by ridesServedToday.
// ---------------------------------------------------------------------------
static void inorderHelper(BSTNode* root, vector<Driver>& out) {
    if (root == NULL) return;
    inorderHelper(root->left, out);
    out.push_back(root->driver);
    inorderHelper(root->right, out);
}

vector<Driver> inorder(BSTNode* root) {
    vector<Driver> out;
    inorderHelper(root, out);
    return out;
}

// ---------------------------------------------------------------------------
//  levelOrder  --  BFS: root first, then each level top-to-bottom, via a queue.
// ---------------------------------------------------------------------------
vector<Driver> levelOrder(BSTNode* root) {
    vector<Driver> out;
    if (root == NULL) return out;

    queue<BSTNode*> q;
    q.push(root);
    while (!q.empty()) {
        BSTNode* n = q.front(); q.pop();
        out.push_back(n->driver);
        if (n->left)  q.push(n->left);
        if (n->right) q.push(n->right);
    }
    return out;
}

// ---------------------------------------------------------------------------
//  searchDriver  --  BST search by key; O(height). Returns NULL if absent.
// ---------------------------------------------------------------------------
Driver* searchDriver(BSTNode* root, int key) {
    BSTNode* cur = root;
    while (cur != NULL) {
        if (key == cur->driver.ridesServedToday) return &cur->driver;
        if (key <  cur->driver.ridesServedToday)  cur = cur->left;
        else                                       cur = cur->right;
    }
    return NULL;
}

// ---------------------------------------------------------------------------
//  freeTree  --  post-order delete of every node.
// ---------------------------------------------------------------------------
void freeTree(BSTNode* root) {
    if (root == NULL) return;
    freeTree(root->left);
    freeTree(root->right);
    delete root;
}
