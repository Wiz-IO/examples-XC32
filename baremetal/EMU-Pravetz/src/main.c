
#include <xc.h>
#include <stdio.h>
#include "user_config.h"
#include "hal_lcd.h"
#include "mcu6502.h"
#include "video.h"

void mcu_process(void);

int main(void)
{
    board_init();
    printf("\nAPPLE ][ 2023 WizIO\n");
    lcd_init(3);

    lcd_fillScreen(LCD_RED);
    delay(100);
    lcd_fillScreen(LCD_GREEN);
    delay(100);
    lcd_fillScreen(LCD_BLUE);
    delay(100);
    lcd_fillScreen(LCD_GRAY);

    mcu_process();

    delay(1000);
    while (1)
    {
        LATEINV = 1 << LED;
        delay(1000);
    }
}

void dump(uint8_t *buf, int len)
{
    printf("[DUMP] %p\n", buf);
    while (len--)
    {
        printf("%02X ", (int)*buf++);
        if (len % 64 == 0)
            printf("\n");
    }
    printf("\n");
}

const char *ascii = "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_ !\"#$%&'()*+,-./0123456789:;<=>?";
void dumpc(uint8_t *buf, int len)
{
    printf("[DUMP] %p\n", buf);
    while (len--)
    {
        printf("%c", ascii[*buf++ & 0x3f]);
        if (len % 16 == 0)
            printf("\n");
    }
    printf("\n");
}

void dumpa(uint8_t *buf, int len)
{
    printf("[DUMP] %p\n", buf);
    while (len--)
    {
        printf("%c", *buf++);
        if (len % 64 == 0)
            printf("\n");
    }
    printf("\n");
}