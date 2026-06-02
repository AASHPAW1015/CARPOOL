// ============================================================================
//  paymentstack.cpp  --  CARPOOL APP  --  LIFO per-person payment stack
// ============================================================================

#include "paymentstack.h"

using namespace std;

// sentinel returned by processNext when the stack is empty (Payment is a
// value type, so we can't return a real null -- customerId -1 means "none").
static const int EMPTY_CUSTOMER = -1;

// ---------------------------------------------------------------------------
//  pushPayments  --  push fares in order; the last one pushed sits on top.
// ---------------------------------------------------------------------------
void pushPayments(stack<Payment>& payStack,
                  const vector<FareItem>& fares) {
    for (size_t i = 0; i < fares.size(); ++i) {
        Payment p;
        p.customerId = fares[i].customerId;
        p.amount     = fares[i].amount;
        p.paid       = false;
        payStack.push(p);
    }
}

// ---------------------------------------------------------------------------
//  processNext  --  pop the top, mark it paid, return it (LIFO order).
// ---------------------------------------------------------------------------
Payment processNext(stack<Payment>& payStack) {
    if (payStack.empty()) {
        Payment none;
        none.customerId = EMPTY_CUSTOMER;
        none.amount     = 0;
        none.paid       = false;
        return none;
    }

    Payment p = payStack.top();
    payStack.pop();
    p.paid = true;
    return p;
}

// ---------------------------------------------------------------------------
//  rollbackAll  --  pop everything in reverse, marking each unpaid; empties it.
// ---------------------------------------------------------------------------
void rollbackAll(stack<Payment>& payStack) {
    while (!payStack.empty()) {
        Payment p = payStack.top();
        payStack.pop();
        p.paid = false;   // undo: this charge is reversed
        // (p goes out of scope; the side effect we're modelling is "not paid")
    }
}

// ---------------------------------------------------------------------------
//  isSentinel  --  true if this is the empty-stack marker from processNext.
// ---------------------------------------------------------------------------
bool isSentinel(const Payment& p) {
    return p.customerId == EMPTY_CUSTOMER;
}
