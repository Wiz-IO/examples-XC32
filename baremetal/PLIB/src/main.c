
#include <xc.h>
#include <stdio.h>
#include <plib/plib_mz.h>

#define SYS_CLK_FREQ (200000000ul)
#define SYS_CLK_BUS_PERIPHERAL_1 (SYS_CLK_FREQ >> 1)
#define LED 4

#define LED_INIT() (ANSELECLR = 1 << LED, TRISECLR = 1 << LED)
#define LED_TOGGLE() LATEINV = 1 << LED

void delay_us(unsigned int us)
{
    us *= SYS_CLK_FREQ / 1000000 / 2;
    _CP0_SET_COUNT(0);
    while (us > _CP0_GET_COUNT())
        continue;
}

void delay_ms(unsigned int ms)
{
    delay_us(ms * 1000);
}

int main(void)
{
    delay_ms(2000); // wait monitor start

    INT_Disable();
    SystemUnlock();

    PcacheSetWaitState(2);
    PcacheSetPrefetchEnable(PCACHE_PREFETCH_ENABLE_ALL);
    SYSTEM_SetCacheCoherency(CACHE_WRITEBACK_WRITEALLOCATE);

    PPSInput(1, U1RX, RPD10);
    PPSOutput(2, RPD15, U1TX);

    SystemLock();
    INT_Enable();

    UARTConfigure(UART1, UART_ENABLE_PINS_TX_RX_ONLY);
    UARTSetLineControl(UART1, UART_DATA_SIZE_8_BITS | UART_PARITY_NONE | UART_STOP_BITS_1);
    UARTSetDataRate(UART1, SYS_CLK_BUS_PERIPHERAL_1, 115200);
    UARTEnable(UART1, UART_ENABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));
    __XC_UART = 1;
    printf("\nPLIB TEST 2023 Georgi Angelov\n");

    LED_INIT();
    while (1)
    {
        LED_TOGGLE();
        delay_ms(1000);
        printf("BEEP ");        
    }
}
