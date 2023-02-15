#include <xc.h>
#include <stdio.h>

#define LED 4
#define SYS_CLK_BUS_PERIPHERAL_1 100000000
#define ReadCoreTimer() _CP0_GET_COUNT()

void Delay(unsigned int ticks)
{
    unsigned int start = ReadCoreTimer(); //
    while ((unsigned int)(ReadCoreTimer() - start) < ticks)
        continue;
}

void uart_init(void)
{
    asm volatile("di");  // disable interrupts
    SYSKEY = 0xAA996655; // unlock
    SYSKEY = 0x556699AA;

    RPD15R = 1; // PPS TX pin - Curiosity microBUS 1

    SYSKEY = 0x33333333; // lock
    asm volatile("ei");  // restore interrupts

    U1STA = U1MODE = 0;
    U1BRG = (((SYS_CLK_BUS_PERIPHERAL_1 >> 4) + (115200 >> 1)) / 115200) - 1;
    U1STA = _U1STA_UTXEN_MASK;
    U1MODE = _U1MODE_ON_MASK;

    __XC_UART = 1; // stdio - UART1

    printf("PIC32MZ Hello World 2023 Georgi Angelov\n");
}

int main(void)
{
    uart_init();
    ANSELECLR = 1 << LED;
    TRISECLR = 1 << LED;
    while (1)
    {
        LATEINV = 1 << LED;
        printf("Go drink beer !!!\n");
        Delay(100000000);
    }
}
