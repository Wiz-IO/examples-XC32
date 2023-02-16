#include <xc.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"

#define SYS_CLK_BUS_PERIPHERAL_1 100000000
#define BRG 115200
#define LED 4

static inline void uart_init(void)
{
    RPD15R = 1;
    U1STA = U1MODE = 0;
    U1BRG = (((SYS_CLK_BUS_PERIPHERAL_1 >> 4) + (BRG >> 1)) / BRG) - 1;
    U1STA = _U1STA_UTXEN_MASK;
    U1MODE = _U1MODE_ON_MASK;
    __XC_UART = 1; // stdio - UART1
}

static void board_init(void)
{
    // Lock
    asm volatile("di");
    SYSKEY = 0xAA996655;
    SYSKEY = 0x556699AA;

    ANSELA = 0;
    ANSELB = 0;
    ANSELC = 0;
    ANSELD = 0;
    ANSELE = 0;
    ANSELF = 0;

    // Use multi vectored interrupts
    _CP0_BIS_CAUSE(0x00800000U);
    INTCONSET = _INTCON_MVEC_MASK;

    // Set up prefetch
    PRECONbits.PFMSECEN = 0;  // Flash SEC Interrupt Enable (Do not generate an interrupt when the PFMSEC bit is set)
    PRECONbits.PREFEN = 0b11; // Predictive Prefetch Enable (Enable predictive prefetch for any address)
    PRECONbits.PFMWS = 2;     // PFM Access Time Defined in Terms of SYSCLK Wait States (Two wait states)

    // SYS_DEVCON_CacheCoherencySet(SYS_CACHE_WRITEBACK_WRITEALLOCATE);

    uart_init();

    // Unlock
    SYSKEY = 0x33333333;
    asm volatile("ei");
}

void Blink(void *arg)
{
    TRISECLR = 1 << LED;
    while (1)
    {
        LATEINV = 1 << LED;
        vTaskDelay(100);
    }
}

void Main(void *arg)
{
    printf("\nPIC32MZ Hello World 2023 Georgi Angelov\n");
    while (1)
    {
        printf("Go drink beer !!!\n");
        vTaskDelay(10000);
    }
}

int main(void)
{
    board_init();
    xTaskCreate(Main, "Main", 1000, NULL, TASK_PRIORITY_NORMAL, NULL);
    xTaskCreate(Blink, "Blink", 400, NULL, TASK_PRIORITY_NORMAL, NULL);
    vTaskStartScheduler();
}

void vAssertCalled(const char *const pcFileName, unsigned long ulLine)
{
    portDISABLE_INTERRUPTS();
    printf("\n[ASSERT] FREERTOS: %s, Line: %lu\n", pcFileName, ulLine);
    while (1)
        continue;
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    portDISABLE_INTERRUPTS();
    printf("\n[ASSERT] FREERTOS Stack Overflow in Task: %s\n", (portCHAR *)pcTaskName);
    while (1)
        continue;
}