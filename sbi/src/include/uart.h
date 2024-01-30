#ifndef __UART_H__
#define __UART_H__

#define UART_MMIO_BASE ((u8*)0x10000000)
#define UART_TXRX      (0)
#define UART_IER       (1)
#define UART_FCR       (2)
#define UART_LCR       (3)
#define UART_LSR       (5)

#define UART_DATA_READY (1 << 0)
#define UART_TX_EMPTY   (1 << 6)

void init_uart(void);
u8   uart_getc(void);
void uart_putc(u8 c);
s64 uart_handle_irq(void);
u8  uart_buffered_getc(void);

#endif
