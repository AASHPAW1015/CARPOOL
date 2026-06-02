// ============================================================================
//  paymentstack.h  --  CARPOOL APP  --  LIFO per-person payment stack
// ============================================================================
//
//  P2's island (the legit "why a stack"): a multi-person payment that fails
//  partway must unwind in REVERSE order. LIFO is the natural fit.
//  push each rider's fare -> pop to pay (processNext) -> rollbackAll if one
//  fails, undoing in exact reverse.
//
// ============================================================================

#ifndef PAYMENTSTACK_H
#define PAYMENTSTACK_H

#include <stack>
#include <vector>
#include "types.h"   // Payment (frozen)

using namespace std;

// one fare to charge: which customer, how much (input to pushPayments)
struct FareItem {
    int    customerId;
    double amount;
};

// --- push each fare as an unpaid Payment, in order (last ends up on top) ---
void pushPayments(stack<Payment>& payStack,
                  const vector<FareItem>& fares);

// --- pop the top payment, mark it paid, return it.
//     on an empty stack returns a sentinel Payment (customerId == -1) ---
Payment processNext(stack<Payment>& payStack);

// --- unwind everything in reverse: pop each, mark unpaid; leaves stack empty.
//     this is the failure path -- undo the whole multi-person charge ---
void rollbackAll(stack<Payment>& payStack);

// --- convenience: did processNext hand back the empty sentinel? ---
bool isSentinel(const Payment& p);

#endif // PAYMENTSTACK_H
