#include <xc.h>
#include <string.h>
#include "mcu6502.h"
#include "GAME_ROM.h"

void mcu_game_load(void)
{
    int page = GAME_ADDRESS >> MCU6502_BANKSHIFT;
    int address = GAME_ADDRESS;
    int size = GAME_SIZE;
    char *src = (char *)GAME;

    int i = 0;
    for (int a = address; a < address + size; a++, i++)
        mem_writebyte(a, GAME[i]);
}

void mcu_game_load_ex(void)
{
    int page = GAME_ADDRESS >> MCU6502_BANKSHIFT;
    int address = GAME_ADDRESS;
    int size = GAME_SIZE;
    char *src = (char *)GAME;

    int i = 0;
    for (int a = address; a < address + size; a++, i++)
        mem_writebyte(a, GAME[i]);

    mcu6502_context *p = mcu6502_get_context();
    p->pc_reg = 0x1998;
    p->a_reg = 0x37;
    p->x_reg = 0x04;
    p->y_reg = 0xBD;
    p->p_reg = 0x34;
    p->s_reg = 0xF6;

    mem_writebyte(0xC055, 0);
    mem_writebyte(0xC050, 0);
}