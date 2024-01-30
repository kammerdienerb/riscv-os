#include "input.h"
#include "utils.h"
#include "sched.h"

#define RING_BUFFER_SIZE (512)


struct {
    Input_Event *head;
    u32          len;
    Input_Event  buff[RING_BUFFER_SIZE];
} ring = {
    .head = ring.buff,
    .len = 0
};


void input_push(Input_Event *event) {
    memcpy(ring.buff + ((ring.head - ring.buff + ring.len) % RING_BUFFER_SIZE), event, sizeof(*event));

    if (ring.len == RING_BUFFER_SIZE) {
        ring.head += 1;
        if (ring.head == ring.buff + RING_BUFFER_SIZE) {
            ring.head = ring.buff;
        }
    } else {
        ring.len += 1;
    }

    sched_try_wake(PROC_WAIT_INPUT);
}


s64 input_pull(Input_Event *event) {
    s64 old_len;

    if (ring.len == 0) { return 0; }

    memcpy(event, ring.head, sizeof(*event));

    ring.head += 1;

    if (ring.head == ring.buff + RING_BUFFER_SIZE) {
        ring.head = ring.buff;
    }

    old_len = ring.len;

    ring.len -= 1;

    return old_len;
}

u32 input_ready(void) { return ring.len; }
