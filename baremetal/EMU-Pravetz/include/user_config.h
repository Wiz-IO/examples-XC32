#pragma once
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#define SYS_CLK_FREQ                        (200000000ul)
#define SYS_CLK_BUS_PERIPHERAL_1            (SYS_CLK_FREQ >> 1)
#define CORE_TIMER_INTERRUPT_TICKS          (SYS_CLK_BUS_PERIPHERAL_1 / 1000ul)
#define CORE_TIMER_TICKS_PER_MICROSECOND    (SYS_CLK_BUS_PERIPHERAL_1 / 1000000ul)
#define UART_BRG                            (115200)
#define LED                                 (4)

extern void (*ms_task)(void);
void board_init(void);
void delay(uint32_t ms);
uint32_t millis(void);
uint32_t micros(void);
void spi_set_pins(void);

#ifdef __cplusplus
}
#endif
