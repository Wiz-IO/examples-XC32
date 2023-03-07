#include <xc.h>
#include <string.h>
#include "mcu6502.h"
#include "GAME_ROM.h"

uint8_t *mcu6502_get_page(int page);
void dumpa(uint8_t *buf, int len);

void mem_writebyte(uint32_t address, uint8_t value);

void mcu_game_load(void)
{
    printf("LOADING PROGRAM [ %d ]\n", GAME_SIZE);
    int page = GAME_ADDRESS >> MCU6502_BANKSHIFT;
    int address = GAME_ADDRESS;
    int size = GAME_SIZE;
    char *src = (char *)GAME;

    int i = 0;
    for (int a = address; a < address + size; a++, i++)
        mem_writebyte(a, GAME[i]);
}