#include <xc.h>
#include <inttypes.h>
#include <string.h>
#include "hal_lcd.h"
#include "mcu6502.h"
#include "video.h"
#include "VIDEO_ROM.h"

enum
{
    S_TEXT = 0x0001,
    S_MIXED = 0x0002,
    S_HIRES = 0x0004,
    S_PAGE2 = 0x0008,
    S_80COL = 0x0010,
    S_ALTCH = 0x0020,
    S_80STORE = 0x0040,
    S_DHIRES = 0x0080
};

static const uint16_t hcolor[] = {
    LCD_BLACK,
    RGBto565(221, 34, 221), // Purple
    RGBto565(17, 221, 0),   // Green
    RGBto565(17, 221, 0),   // Green
    RGBto565(221, 34, 221), // Purple
    RGBto565(34, 34, 255),  // Medium Blue ?
    RGBto565(255, 102, 0),  // Orange
    RGBto565(255, 102, 0),  // Orange
    RGBto565(34, 34, 255),  // Medium Blue ?
    LCD_WHITE,
};

static const uint16_t lcolor[16] = {
    RGBto565(0, 0, 0),       // Black
    RGBto565(221, 0, 51),    // Magenta
    RGBto565(0, 0, 153),     // Dark Blue
    RGBto565(221, 34, 221),  // Purple
    RGBto565(0, 119, 34),    // Dark Green
    RGBto565(85, 85, 85),    // Grey 1
    RGBto565(34, 34, 255),   // Medium Blue
    RGBto565(102, 170, 255), // Light Blue
    RGBto565(136, 85, 0),    // Brown
    RGBto565(255, 102, 0),   // Orange
    RGBto565(170, 170, 170), // Grey 2
    RGBto565(255, 153, 136), // Pink
    RGBto565(17, 221, 0),    // Green
    RGBto565(255, 255, 0),   // Yellow
    RGBto565(68, 255, 153),  // Aquamarine
    RGBto565(255, 255, 255), // White
};

static uint16_t switches = 0;

static uint16_t scan_line = 0;

static uint16_t __attribute__((aligned(4))) video_frame_buffer[VIDEO_BUFFER_SIZE] = {0};

static void (*video_draw_scan_line)(uint16_t *line_data, uint16_t scanline, int x, int y) = NULL;

uint8_t video_update_switches(uint16_t address)
{
    switch (address)
    {
    case 0xC050: // CLRTEXT
        if (switches & S_TEXT)
        {
            switches &= ~S_TEXT;
        }
        break;
    case 0xC051: // SETTEXT
        if (!(switches & S_TEXT))
        {
            switches |= S_TEXT;
        }
        break;
    case 0xC052: // CLRMIXED
        if (switches & S_MIXED)
        {
            switches &= ~S_MIXED;
        }
        break;
    case 0xC053: // SETMIXED
        if (!(switches & S_MIXED))
        {
            switches |= S_MIXED;
        }
        break;
    case 0xC054: // PAGE1
        if (switches & S_PAGE2)
        {
            switches &= ~S_PAGE2;
            if (!(switches & S_80COL))
            {
            }
            else
            {
                // updateMemoryPages();
            }
        }
        break;
    case 0xC055: // PAGE2
        if (!(switches & S_PAGE2))
        {
            switches |= S_PAGE2;
            if (!(switches & S_80COL))
            {
            }
            else
            {
                // updateMemoryPages();
            }
        }
        break;
    case 0xC056: // CLRHIRES
        if (switches & S_HIRES)
        {
            switches &= ~S_HIRES;
        }
        break;
    case 0xC057: // SETHIRES
        if (!(switches & S_HIRES))
        {
            switches |= S_HIRES;
        }
        break;
    case 0xC05E: // DHIRES ON
        if (!(switches & S_DHIRES))
        {
            switches |= S_DHIRES;
        }
        break;
    case 0xC05F: // DHIRES OFF
        if (switches & S_DHIRES)
        {
            switches &= ~S_DHIRES;
        }
        break;
    } // switch
    // printf("%04X %04X\n", address, switches);
    return 0;
}

static void video_draw_T(uint8_t *video_line_data, int x, int y)
{
    static uint32_t T = 0;
    static bool flash = false;
    if (millis() - T > 500)
    {
        T = millis();
        flash = !flash;
    }

    uint16_t text_char = video_line_data[x];
    if (text_char == 0x60)
        text_char = (flash) ? 0x60 : 0xff;

    uint16_t rom_char = text_char * TEXT_BYTES;
    uint16_t rom_char_offset = y & TEXT_BYTES_MASK;
    uint16_t data = ROM_CHAR[rom_char + rom_char_offset];

    for (int i = 0; i < 7; i++)
    {
        uint8_t color = 9; // WHITE INDEX
        uint8_t pixel = (data >> (i)) & 1;
        if (pixel)
            color = 0;
        video_frame_buffer[x * 7 + i] |= hcolor[color];
    }
}

static void video_draw_L(uint8_t *video_line_data, int x, int y)
{
    int color = video_line_data[x] >> 4;
    for (int i = 0; i < 7; i++)
        video_frame_buffer[x * 7 + i] = lcolor[color];
}

static void video_draw_H(uint8_t *video_line_data, int x, int y)
{
    uint16_t data = video_line_data[x];
    uint16_t pixel_pre = (video_line_data[x - 1] & 0x60) >> 5;
    uint16_t pixel_post = video_line_data[x + 1] & 3;
    uint8_t address_odd = (x & 1) << 1;
    uint8_t color_offset = (data & 0x80) >> 5;
    uint16_t data_extended = pixel_pre + ((data & 0x7F) << 2) + (pixel_post << 9);
    for (int i = 0; i < 7; i++)
    {
        uint8_t color = 0; // BLACK INDEX
        uint8_t pixel = (data_extended >> (i + 2)) & 1;
        uint8_t pixel_left1 = (data_extended >> (i + 1)) & 1;
        uint8_t pixel_right1 = (data_extended >> (i + 3)) & 1;
        if (pixel)
        {
            if (pixel_left1 || pixel_right1)
            {
                color = 9; // WHITE INDEX
            }
            else
            {
                color = color_offset + address_odd + (i & 1) + 1;
            }
        }
        else
        {
            if (pixel_left1 && pixel_right1)
            {
                color = color_offset + address_odd + 1 - (i & 1) + 1;
            }
        }
        video_frame_buffer[x * 7 + i] |= hcolor[color];
    }
}

static uint16_t video_get_address(bool H, uint8_t y)
{
    if (H)
    {
        return 0x2000 + (0x2000 * (bool)(switches & S_PAGE2)) + (y & 7) * 0x400 + ((y >> 3) & 7) * 0x80 + (y >> 6) * 0x28;
    }
    else
    {
        return 0x400 + (0x400 * (bool)(switches & S_PAGE2)) + (((y >> 3) & 0x07) * SCREEN_LINE_OFFSET) + ((y >> 6) * VIDEO_SEGMENT_OFFSET);
    }
}

static void video_draw(uint8_t y)
{
    if (y < VIDEO_RESOLUTION_Y)
    {
        memset(video_frame_buffer, 0, VIDEO_BUFFER_SIZE * 2);
        uint8_t *mem_txt = mem_get_ptr(video_get_address(0, y));
        uint8_t *mem_hgr = mem_get_ptr(video_get_address(1, y));
        for (int x = 0; x < VIDEO_BYTES_PER_LINE; x++)
        {
            if (switches & S_TEXT)
            {
                video_draw_T(mem_txt, x, y);
            }
            else if (switches & S_MIXED)
            {
                if (switches & S_HIRES)
                {
                    // HGR
                    if (y < 160)
                        video_draw_H(mem_hgr, x, y);
                    else
                        video_draw_T(mem_txt, x, y);
                }
                else
                {
                    // GR
                    if (y < 160)
                        video_draw_L(mem_txt, x, y);
                    else
                        video_draw_T(mem_txt, x, y);
                }
            }
            else
            {
                // HGR2
                video_draw_H(mem_hgr, x, y);
            }
        }

        if (video_draw_scan_line)
            video_draw_scan_line(video_frame_buffer, y, VIDEO_RESOLUTION_X, VIDEO_RESOLUTION_Y);
    }
}

///////////////////////////////////////////////////////////////////////////////

uint32_t video_render_line(int y)
{
    uint32_t begin = micros();
    scan_line = y;
    video_draw(scan_line);
    if (++scan_line > 192)
    {
        scan_line = 0;
        LATEINV = 1 << 4;
    }
    return micros() - begin; // 102 uS
}

uint32_t video_render_screen(void)
{
    uint32_t begin = millis();
    for (int y = 0; y < VIDEO_RESOLUTION_Y; y++)
        video_draw(y);
    LATEINV = 1 << 4;
    return millis() - begin; // 25 mS
}

void video_init(void)
{
    video_draw_scan_line = lcd_draw_scan_line;
}