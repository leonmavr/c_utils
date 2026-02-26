#ifndef OBSERVABLE_CLOCK_H_
#define OBSERVABLE_CLOCK_H_

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h> // size_t

// Generic callback:
// Pointer to func with void return with void* input
typedef void (*callback_t)(void *data);
typedef struct ObserverNode ObserverNode;

// Observer:
// stores a callback and a next point to allow traversal
struct ObserverNode {
    callback_t callback; // callback function pointer
    void *data;          // observer-specific data passed to callback
    ObserverNode *next;
};

// Observable subject:
// - square wave toggling every period/2
// - maintains a linked list of observers
typedef struct  {
    bool signal;              // low or high
    size_t period;            // in arbitrary units (e.g. ms)
    size_t time;              // in arbitrary units
    ObserverNode *observers;  // head of observer linked list
} Clock;


static inline void clock_init(Clock *clk, size_t period) {
    clk->period = period;
    clk->time = 0;
    clk->signal = true;
    clk->observers = NULL;
}

// Add an observer node to the clock's list.
// Caller owns the Observer storage (stack/global/heap); Clock only links it.
static inline void clock_add_observer(Clock *clk, ObserverNode *observer) {
    // Before:
    // (head of the list)                                new node:
    // +-----------------+                               +------+
    // | clk observers   |----+                          |  X   |
    // +-----------------+    |                          | next |---+
    //                        |                          +------+   |
    //                        v                                     v
    //                    +------+    +------+                     ???  
    //                    |  A   |    |  B   |
    //                    | next |--->| next |--->NULL
    //                    +------+    +------+
    //
    // observer->next = clk->observers:
    // +------+
    // |  X   |
    // | next |---+
    // +------+   |
    //            |
    //            v                         +------+
    // +-----------------+    effectively   |  X   |
    // | clk observers   |  ==============> | next |
    // +-----------------+                  +------+
    //   |                                   |
    //   |   +------+   +------+             |   +------+   +------+
    //   |   |  A   |   |  B   |             |   |  A   |   |  B   |
    //   +-->| next |-->| next |             +-->| next |-->| next |
    //       +------+   +------+                 +------+   +------+
    //
    // clk->observers = observer:
    // +-----------------+ 
    // | clk observers   |
    // +-----------------+
    //   |
    //   |  +------+   +------+   +------+   
    //   +->|  X   |   |  A   |   |  B   |   
    //      | next |-->| next |-->| next |-->NULL
    //      +------+   +------+   +------+   
    if (!clk || !observer) return;
    observer->next = clk->observers;
    clk->observers = observer;
}

static inline void clock_remove_observer(Clock* clk, ObserverNode *observer) {
    if (!clk || !observer) return;
    ObserverNode** current = &clk->observers;
    while (*current) {
        if (*current == observer) {
            *current = observer->next; // unlink the observer
            break;
        }
        current = &(*current)->next;
    }
}

static void clock_notify_listeners(Clock *clk) {
    // Invoke the callback of each registered observer
    if (!clk) return;
    for (ObserverNode *obs = clk->observers; obs != NULL; obs = obs->next) {
        if (obs->callback && obs->data)
            obs->callback(obs->data);
    }
}

static inline void clock_tick(Clock *clk, size_t delta_time) {
    if (!clk || clk->period == 0) return;
    clk->time += delta_time;
    clk->signal = (clk->time % clk->period) < (clk->period / 2);
    if (clk->time >= clk->period) {
        clk->time = 0;
        clock_notify_listeners(clk);
    }
}



#if 0
// example:
void foo(void *data) {
    const char *s = (const char *)data;
    printf("Callback from foo with data: %s\n", s);
}

void bar(void *data) {
    const int *s = (const int *)data;
    printf("Callback from bar with data: %d\n", *s);
}

// ...
Clock clk;
clock_init(&clk, 100);
ObserverNode node1 = { .callback = foo,
                       .data = "Somebody's watching you!\n",
                       .next = NULL };
int i = 42;
ObserverNode node2 = { .callback = bar, .data = &i, .next = NULL };
clock_add_observer(&clk, &node1);
clock_add_observer(&clk, &node2);
for (int t = 0; t < 400; t += 25) {
    clock_tick(&clk, 25);
#endif

#endif // OBSERVABLE_CLOCK_H_

