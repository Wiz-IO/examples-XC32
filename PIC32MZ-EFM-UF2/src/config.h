#ifndef CONFIG_H
#define CONFIG_H

///////////////////////////////////////////////////////////////////////////////

#define APP_JUMP_ADDRESS            (0x1D000200UL)
#define APP_START_ADDRESS           (0x1D000000UL)
#define APP_DOWNLOAD_ENABLE

#define NVM_FLASH_SIZE              (0x200000UL)
#define NVM_FLASH_PAGESIZE          (16384UL)

///////////////////////////////////////////////////////////////////////////////

#define UF2_VERSION                 "1.0"
#define UF2_PRODUCT_NAME            "PIC32MZ"
#define UF2_BOARD_ID                "0xAABBCCDD"
#define UF2_INDEX_URL               "https://github.com/Wiz-IO"
#define UF2_VOLUME_LABEL            "UF2-PIC32MZ"

#define BOARD_UF2_FAMILY_ID         0xAABBCCDD

///////////////////////////////////////////////////////////////////////////////

//#define DBG

#ifdef DBG
#include <stdio.h>
#define PRINT printf
#define PRINT_FUNC printf("%s()\n", __FUNCTION__)
#define BRG 115200
#else
#define PRINT
#define PRINT_FUNC
#endif

#define SYS_CLK_FREQ 200000000u

#define BUTTON 12
#define BUTTON_INIT()            \
    do                           \
    {                            \
        ANSELGCLR = 1 << BUTTON; \
        TRISGSET = 1 << BUTTON;  \
    } while (0)
#define LED 4

#define LED_INIT()            \
    do                        \
    {                         \
        ANSELECLR = 1 << LED; \
        TRISECLR = 1 << LED;  \
    } while (0)

#define LED_TOGGLE() LATEINV = 1 << LED
#define LED_HI() LATESET = 1 << LED
#define LED_LO() LATECLR = 1 << LED

///////////////////////////////////////////////////////////////////////////////

#endif