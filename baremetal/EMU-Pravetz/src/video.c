#include <xc.h>
#include <inttypes.h>
#include <string.h>
#include "hal_lcd.h"
#include "mcu6502.h"
#include "video.h"

#include "VIDEO_ROM.h"

enum HCOLOR
{
    BLACK = 0,
    PURPLE,
    GREEN,
    GREEN_1,
    PURPLE_1,
    BLUE,
    ORANGE_0,
    ORANGE_1,
    BLUE_1,
    WHITE,
    HCOLOR_LENGTH,
};

const uint16_t hcolor[] = {
    LCD_BLACK,
    LCD_A_PURPLE,
    LCD_A_GREEN,
    LCD_A_GREEN,
    LCD_A_PURPLE,
    LCD_A_BLUE,
    LCD_A_ORANGE,
    LCD_A_ORANGE,
    LCD_A_BLUE,
    LCD_WHITE,
};

typedef enum
{
    VIDEO_TEXT_MODE = 0,
    VIDEO_LRES = 1,    
    VIDEO_GRAPHICS_MODE = 2,
    VIDEO_HRES = 3,

    VIDEO_PAGE_1 = 0,
    VIDEO_PAGE_2 = 1,
} VideoModes;

static uint8_t video_char_set[CHARACTER_SET_ROM_SIZE];
static uint16_t video_buffer[VIDEO_BUFFER_SIZE] = {0};
static uint8_t video_mode = VIDEO_TEXT_MODE;
static uint8_t video_page = VIDEO_PAGE_1;
static uint16_t video_scan_line;

static unsigned char reverse(unsigned char b)
{
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return ~b >> 1;
}

void video_init(void)
{
    memcpy(video_char_set, ROM_CHAR, CHARACTER_SET_ROM_SIZE);
}

void video_update(uint16_t address)
{
    if (address >= 0xC050 && address < 0xC058)
    {
        printf("VIDEO MODE: %04X\n", (int)address);
    }
    // https://www.kreativekorp.com/miscpages/a2info/iomemory.shtml
    switch (address)
    {
    case 0xC050: // Display Graphics
        video_mode = VIDEO_GRAPHICS_MODE;
        break;
    case 0xC051: // Display Text
        video_mode = VIDEO_TEXT_MODE;
        break;
    case 0xC052: // Display Full Screen
        break;
    case 0xC053: // Display Split Screen
        break;
    case 0xC054: // Display Page 1
        video_page = VIDEO_PAGE_1;
        break;
    case 0xC055: // Display Page 2
        video_page = VIDEO_PAGE_2;
        break;
    case 0xC056: // Display LoRes Graphics
        video_mode |= VIDEO_LRES;
        break;
    case 0xC057: // Display HiRes Graphics
        video_mode |= VIDEO_HRES;
        break;
    }
}

void video_hires_line_update(uint16_t video_line_number, uint8_t *video_line_data)
{
    uint16_t data = 0;
    uint16_t pixel_pre = 0;
    uint16_t pixel_post = 0;
    uint16_t data_extended = 0;
    uint8_t address_odd = 0;
    uint8_t color_offset = 0;
    uint8_t color = 0;
    uint8_t pixel = 0;
    uint8_t pixel_left1 = 0;
    uint8_t pixel_right1 = 0;

    if (video_line_number < VIDEO_RESOLUTION_Y)
    {
        for (int j = 0; j < VIDEO_BYTES_PER_LINE; j++)
        {
            data = video_line_data[j];
            pixel_pre = (video_line_data[j - 1] & 0x60) >> 5;
            pixel_post = video_line_data[j + 1] & 3;
            address_odd = (j & 1) << 1;
            color_offset = (data & 0x80) >> 5;
            data_extended = pixel_pre + ((data & 0x7F) << 2) + (pixel_post << 9);
            for (int i = 0; i < 7; i++)
            {
                color = BLACK;
                pixel = (data_extended >> (i + 2)) & 1;
                pixel_left1 = (data_extended >> (i + 1)) & 1;
                pixel_right1 = (data_extended >> (i + 3)) & 1;
                if (pixel)
                {
                    if (pixel_left1 || pixel_right1)
                    {
                        color = WHITE;
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
                video_buffer[j * 7 + i] |= hcolor[color];
            }
        }
    }
}

void video_text_line_update(uint16_t video_line_number, uint8_t *video_line_data)
{
    uint16_t text_char = 0;
    uint16_t data = 0;
    uint8_t color = 0;
    uint8_t pixel_color = WHITE;
    uint8_t pixel = 0;
    uint16_t rom_char = 0;
    uint16_t rom_char_offset = 0;
    if (video_line_number < VIDEO_RESOLUTION_Y)
    {
        for (int j = 0; j < VIDEO_BYTES_PER_LINE; j++)
        {
            text_char = video_line_data[j];

#if 0
            if ((text_char & 0xC0) == 0x40)
            {
                text_char &= 0x3F;
                if (text_char < 0x20)
                {
                    text_char += 0x40;
                }
            }
#endif
            rom_char = text_char * TEXT_BYTES;
            rom_char_offset = video_line_number & TEXT_BYTES_MASK;
            data = video_char_set[rom_char + rom_char_offset];

#ifdef CHARSET_REVERSE
            // reversed with pytnon script
            // data = reverse(data); // ПРАВЕЦ ... optimization ?
#endif

            for (int i = 0; i < 7; i++)
            {
                color = pixel_color;
                pixel = (data >> (i)) & 1;
                if (pixel)
                    color = BLACK;
                video_buffer[j * 7 + i] |= hcolor[color];
            }
        }
    }
}

void video_scan_line_set(uint16_t line)
{
    video_scan_line = line;
}

void video_buffer_clear(void)
{
    memset(video_buffer, 0, VIDEO_BUFFER_SIZE * 2);
}

void video_buffer_get(uint16_t *buffer)
{
    memcpy(buffer, video_buffer, VIDEO_BUFFER_SIZE * 2);
}

uint16_t video_address_get(void)
{
    uint16_t address;
    if (video_mode == VIDEO_TEXT_MODE)
    {
        address = 0x400 + (0x400 * video_page) +
                  (((video_scan_line >> 3) & 0x07) * SCREEN_LINE_OFFSET) +
                  ((video_scan_line >> 6) * VIDEO_SEGMENT_OFFSET);
    }
    else
    {
        address = 0x2000 + (0x2000 * video_page) +
                  (video_scan_line & 7) * 0x400 +
                  ((video_scan_line >> 3) & 7) * 0x80 +
                  (video_scan_line >> 6) * 0x28;
    }
    return address;
}

void video_line_data_get(uint8_t *video_line_data)
{
    if (video_mode < VIDEO_GRAPHICS_MODE)
    {
        video_text_line_update(video_scan_line, video_line_data);
    }
    else
    {
        video_hires_line_update(video_scan_line, video_line_data);
    }
}

///////////////////////////////////////////////////////////////////////////////

static uint8_t video_line_data[VIDEO_BYTES_PER_LINE] = {0}; // [40]
static uint16_t video_address = 0x2000;
static uint16_t scan_line = 0;

#define VIDEO_RESOLUTION_X 280
#define VIDEO_RESOLUTION_Y 192
static uint16_t video_buffer[VIDEO_RESOLUTION_X];
static void draw_scanline(uint16_t *bmp, int scanline)
{
    if (scanline < 0 || scanline >= 240)
        return;
    int xoffset = (320 - VIDEO_RESOLUTION_X) / 2;
    int yoffset = (240 - VIDEO_RESOLUTION_Y) / 2;
    lcd_drawImage(xoffset, scanline + yoffset, VIDEO_RESOLUTION_X, 1, bmp);
}

void lcd_render_line(void)
{
    video_scan_line_set(scan_line);
    video_buffer_clear();
    video_address = video_address_get();
    memcpy(video_line_data, bank_get_mem(video_address), VIDEO_BYTES_PER_LINE);
    video_line_data_get(video_line_data);
    video_buffer_get(video_buffer);
    draw_scanline(video_buffer, scan_line);
    if (++scan_line > 192)
    {
        scan_line = 0;
        LATEINV = 1 << 4;
    }
}