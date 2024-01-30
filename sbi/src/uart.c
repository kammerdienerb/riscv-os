#include "common.h"
#include "uart.h"
#include "lock.h"

#include <stdarg.h>

static volatile u8 *_uart = UART_MMIO_BASE;


#define RING_BUFFER_SIZE (32)


struct {
    u8      *head;
    u32      len;
    u8       buff[RING_BUFFER_SIZE];
    Spinlock lock;
} ring;

static void ring_init(void) {
    ring.head   = ring.buff;
    ring.len    = 0;
    ring.lock.s = SPIN_UNLOCKED;
}

static void ring_push(u8 c) {
    spin_lock(&ring.lock);

    ring.buff[(ring.head - ring.buff + ring.len) % RING_BUFFER_SIZE] = c;

    if (ring.len == RING_BUFFER_SIZE) {
        ring.head += 1;
        if (ring.head == ring.buff + RING_BUFFER_SIZE) {
            ring.head = ring.buff;
        }
    } else {
        ring.len += 1;
    }

    spin_unlock(&ring.lock);
}

static u8 ring_pop(void) {
    u8 c;

    spin_lock(&ring.lock);

    c = 255;

    if (ring.len == 0) { goto out; }

    c = *ring.head;

    ring.head += 1;

    if (ring.head == ring.buff + RING_BUFFER_SIZE) {
        ring.head = ring.buff;
    }

    ring.len -= 1;

out:;
    spin_unlock(&ring.lock);

    return c;
}




u8 uart_getc(void) {
    if (_uart[UART_LSR] & UART_DATA_READY) {
        return _uart[UART_TXRX];
    }

    return 0xff;
}

void uart_putc(u8 c) {
    while (!(_uart[UART_LSR] & UART_TX_EMPTY));

    _uart[UART_TXRX] = c;
}

void init_uart(void) {
    _uart[UART_LCR] = 0x3; /* Word length select bits */
    _uart[UART_FCR] = 1;   /* enable FIFO             */
    _uart[UART_IER] = 1;   /* enable interrupts       */

    ring_init();
}

s64 uart_handle_irq(void) {
    u8 c;

    c = uart_getc();

    ring_push(c);

    return 0;
}

u8  uart_buffered_getc(void) {
    return ring_pop();
}
