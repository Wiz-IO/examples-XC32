#include <xc.h>
#include <string.h>
#include "mcu6502.h"
#include "ROM_DATA.h"
#include "hal_lcd.h"
#include "video.h"
#include "programs.h"

void mcu_game_load(void);
void mcu_game_load_ex(void);

// MAPER //////////////////////////////////////////////////////////////////////

static uint8_t key = 0x80;
static uint32_t keyCount = 0;
static uint8_t last = 0x00;

#define LAST_MEMORY_HANDLER \
    {                       \
        -1, -1, NULL        \
    }

static uint8_t get_io(uint32_t address)
{
    // printf("io_read:  %04X\n", (int)address);
    uint8_t result = 0;
    if (0xC000 == address) // Last Key Pressed + 128
    {
        keyCount++;
        if (keyCount < 0x800)
        {
            result = key;
        }
        else if (keyCount & 0xFF)
        {
            result = key;
        }
        else
        {
            uint32_t n = (keyCount - 0x800) >> 8;
            if (n < sizeof(PROGRAM))
                key = PROGRAM[n] | 0x80;
            result = key;
        }

        if (!PORTGbits.RG12)
        {
            key = ' '|0x80;
            result = key;
        }
    }
    else if (0xC010 == address) // Keyboard Strobe
    {
        key &= 0x7F;
        result = 0;
    }
    return result;
}

static void set_io(uint32_t address, uint8_t value)
{
    if (0xC000 == address) // I/O emulation.
        key = value;
}

static uint8_t get_video(uint32_t address) { return video_update_switches(address); }

static void set_video(uint32_t address, uint8_t value) { video_update_switches(address); }

static mcu6502_memread default_read_handler[] = {
    {0xC000, 0xC04F, get_io},
    {0xC050, 0xC05F, get_video},
    {0xC050, 0xCFFF, get_io},
    LAST_MEMORY_HANDLER,
};

static mcu6502_memwrite default_write_handler[] = {
    {0xC000, 0xC04F, set_io},
    {0xC050, 0xC05F, set_video},
    {0xC050, 0xCFFF, set_io},
    LAST_MEMORY_HANDLER,
};

///////////////////////////////////////////////////////////////////////////////

void mcu_init(const uint8_t *rom, uint32_t address)
{
    mcu6502_context p;
    mcu6502_getcontext(&p);
    switch (address)
    {
    case 0xD000:
        p.memory = calloc(MCU6502_BANKSIZE * 16, 1);
        memcpy(&p.memory[address], rom, MCU6502_BANKSIZE * 3);
        p.read_handler = default_read_handler;
        p.write_handler = default_write_handler;
        break;

    default:
        printf("[ERROR] Invalid ROM\n");
        abort();
        break;
    }

    mcu6502_setcontext(&p);
}

const unsigned char MCU_ROM_DATA[]; // ROM_DATA.h

static void checkfiq(int cycles) // need ?
{
    static uint32_t fiq_cycles = 0;
    static bool fiq_occurred = 0;
    static uint8_t fiq_state = 0;
    fiq_cycles -= cycles;
    if (fiq_cycles <= 0)
    {
        fiq_cycles += 10;
        if (0 == (fiq_state & 0xC0))
        {
            fiq_occurred = true;
            mcu6502_irq();
        }
    }
}

void mcu_process(void)
{
    uint32_t timer = 0;

    video_init(); // TODO driver

    mcu_init(MCU_ROM_DATA, MCU_ROM_ADDRESS);
    mcu6502_reset();

#ifdef RUN_GAME
    mcu_game_load();
#endif

    while (1)
    {
        int elapsed_cycles = mcu6502_execute(16); // ?
        // checkfiq(elapsed_cycles);

        if (millis() - timer > 20) // ? 50 Hz
        {
            video_render_screen(); // 25 mSec ( 40 frames )
            timer = millis();
        }
    }
}