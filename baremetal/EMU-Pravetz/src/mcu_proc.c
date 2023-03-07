#include <xc.h>
#include <string.h>
#include "mcu6502.h"
#include "ROM_DATA.h"
#include "hal_lcd.h"
#include "video.h"
#include "programs.h"
void mcu_game_load(void);

// MAPER //////////////////////////////////////////////////////////////////////

static uint8_t key = 0x80;
static uint32_t keyCount = 0;
static uint8_t last = 0x00;

#define LAST_MEMORY_HANDLER \
    {                       \
        -1, -1, NULL        \
    }

static uint8_t m1_read(uint32_t address)
{
    // printf("m1_read:  %04X\n", (int)address);
    return last;
}

static uint8_t m2_read(uint32_t address)
{
    // printf("m2_read:  %04X\n", (int)address);
    return 0xFF; // Bus error due to minimum memory installation.
}

static uint8_t io_read(uint32_t address)
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
    }
    else if (0xC010 == address) // Keyboard Strobe
    {
        key &= 0x7F;
        result = 0;
    }
    else
    {
        // printf("io_read:  %04X\n", (int)address);
        video_update(address);
    }
    return result;
}

static void m1_write(uint32_t address, uint8_t value)
{
    // printf("m1_write: %04X = %02X\n", (int)address, (int)value);
    last = value; // Fake memory impl to make it run on low memory chip.
}

static void m2_write(uint32_t address, uint8_t value)
{
    // printf("m2_write: %04X = %02X\n", (int)address, (int)value);
}

static void io_write(uint32_t address, uint8_t value)
{
    // printf("io_write: %04X = %02X\n", (int)address, (int)value);
    if (0xC000 == address) // I/O emulation.
        key = value;
    else
        video_update(address);
}

static mcu6502_memread default_readhandler[] = {
    //{0x0300, 0x03FF, m1_read},
    //{0x0400, 0x07FF, m2_read},
    //{0x0900, 0x0FFF, m1_read},
    //{0x1000, 0xBFFF, m2_read},
    {0xC000, 0xCFFF, io_read},
    LAST_MEMORY_HANDLER,
};

static mcu6502_memwrite default_writehandler[] = {
    //{0x0300, 0x03FF, m1_write},
    //{0x0400, 0x07FF, m2_write},
    //{0x0900, 0x0FFF, m1_write},
    //{0x1000, 0xBFFF, m2_write},
    {0xC000, 0xCFFF, io_write},
    LAST_MEMORY_HANDLER,
};

///////////////////////////////////////////////////////////////////////////////

void mcu_init(const uint8_t *rom, uint32_t address)
{
    mcu6502_context mmc_cpu;
    mcu6502_getcontext(&mmc_cpu);

    int page;
    page = address >> MCU6502_BANKSHIFT; // 12
    printf("PAGE: %d\n", page);
    for (int i = 0; i < page; i++)
    {
        mmc_cpu.mem_page[i] = calloc(MCU6502_BANKSIZE, 1); // create RAM
        if (NULL == mmc_cpu.mem_page[i])
        {
            printf("[ERROR] MALLOC\n");
            abort();
        }
        else
        {
            printf("CREATE RAM: %02d %p\n", i, mmc_cpu.mem_page[i]);
        }
    }

    switch (address)
    {
    case 0xD000:
        mmc_cpu.mem_page[page + 0] = (uint8_t *)&rom[0 * MCU6502_BANKSIZE]; // x 4096
        mmc_cpu.mem_page[page + 1] = (uint8_t *)&rom[1 * MCU6502_BANKSIZE];
        mmc_cpu.mem_page[page + 2] = (uint8_t *)&rom[2 * MCU6502_BANKSIZE];
        mmc_cpu.read_handler = default_readhandler;
        mmc_cpu.write_handler = default_writehandler;
        mcu6502_setcontext(&mmc_cpu);
        break;

    default:
        printf("[ERROR] Invalid ROM\n");
        abort();
        break;
    }
    // dumpc(rom, MCU6502_BANKSIZE*3);
}

const unsigned char MCU_ROM_DATA[]; // ROM_DATA.h

static void checkfiq(int cycles)
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
    static int cnt = 0;
    video_init();
    mcu_init(MCU_ROM_DATA, MCU_ROM_ADDRESS);
    mcu6502_reset();
    mcu_game_load();
    while (1)
    {
        int elapsed_cycles = mcu6502_execute(16);
        checkfiq(elapsed_cycles);

        if (cnt % 100 == 0) // timer ?
            lcd_render_line();
        cnt++;
    }
}