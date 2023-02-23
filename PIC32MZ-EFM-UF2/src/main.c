
#include <xc.h>
#include <sys/kmem.h>
#include "config.h"
#include "USB.h"
#include "NVM.h"
#include "uf2.h"

#ifdef DBG

static inline void uart_init(void)
{
    RPD15R = 1;
    U1STA = U1MODE = 0;
    U1BRG = ((((SYS_CLK_FREQ >> 1) >> 4) + (BRG >> 1)) / BRG) - 1;
    U1STA = _U1STA_UTXEN_MASK;
    U1MODE = _U1MODE_ON_MASK;
    __XC_UART = 1; // stdio - UART1
    printf("\nDEBUG MODE\n");
}

void dump(uint32_t *buf, int len)
{
    printf("[DMP]\n");
    while (len--)
    {
        printf("%08X ", (int)*buf++);
        if (len % 4 == 0)
            printf("\n");
    }
    printf("\n");
}

#endif

void delay_us(unsigned int us)
{
    us *= SYS_CLK_FREQ / 1000000 / 2; // Core Timer updates every 2 ticks
    _CP0_SET_COUNT(0);
    while (us > _CP0_GET_COUNT())
        continue;
}

void delay_ms(unsigned int ms) { delay_us(ms * 1000); }

void boot(void)
{
#define LED_COUNT 10000000UL
#define VBUS_VALID 3

    uint32_t led_count = _CP0_GET_COUNT();
    LED_INIT();
    uf2_init();
    NVM_Initialize();
    USB_EJECTED = USB_POWERED = USB_EJECT_REQUEST = 0;
    CFGCONbits.USBSSEN = 1;
    delay_ms(1);
    while (!USB_EJECTED)
    {
        USB_handler();

        if ((USBOTGbits.VBUS == VBUS_VALID) && !USB_POWERED && !USB_EJECTED)
        {
            USB_init();
            CNPUFbits.CNPUF3 = 1; // USBID ?
            USB_connect();
            USB_POWERED = 1;
        }

        if (USB_POWERED)
        {
            USB_tasks();
        }

        if (USB_EJECTED || (USBOTGbits.VBUS != VBUS_VALID && USB_POWERED))
        {
            USB_disconnect();
            break;
        }

        if (_CP0_GET_COUNT() - led_count > LED_COUNT)
        {
            led_count = _CP0_GET_COUNT();
            LED_TOGGLE();
        }
    }

    CFGCONbits.USBSSEN = 0;
    PRINT("BOOT END %d\n", USB_EJECTED);
    delay_ms(50);
#ifdef DBG
    U1MODE = 0;
    __XC_UART = 0;
#endif
}

static inline void board_init(void)
{
#ifndef DBG
    BUTTON_INIT();
    if ((PORTG & (1 << BUTTON)))
        return;
#endif

    SYSKEY = 0xAA996655;
    SYSKEY = 0x556699AA;
#ifdef DBG
    uart_init();
#endif

    boot();
}

int main(void)
{
    board_init();

    void (*run)(void) = (void (*)(void))((uint32_t)(PA_TO_KVA0(APP_JUMP_ADDRESS)));
    if (*(int *)run != 0xFFFFFFFF)
        run();

    LED_INIT();
    while (1)
    {
        LATEINV = 1 << LED;
        delay_ms(1000);
    }
}
