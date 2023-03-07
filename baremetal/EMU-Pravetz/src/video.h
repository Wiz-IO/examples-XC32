#ifndef __VIDEO_H__
#define __VIDEO_H__

#include <stdbool.h>
#include <stdint.h>

#define VIDEO_BYTES_PER_LINE    40
#define SCREEN_LINE_OFFSET      0x80
#define VIDEO_SEGMENT_OFFSET    0x28

#define VIDEO_RESOLUTION_X      280
#define VIDEO_RESOLUTION_Y      192
#define VIDEO_BUFFER_SIZE       (VIDEO_RESOLUTION_X)

#define TEXT_BYTES              8
#define TEXT_BYTES_MASK         0x07

//https://github.com/newdigate/rgb565_colors

#define RGBto565(r,g,b)         ((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)) 

#define LCD_BLACK               0x0000
#define LCD_RED                 RGBto565(0xFF, 0x00, 0x00) 
#define LCD_GREEN               RGBto565(0x00, 0xFF, 0x00) 
#define LCD_BLUE                RGBto565(0x00, 0x00, 0xFF) 
#define LCD_GRAY                RGBto565(0x40, 0x40, 0x40) 

#define LCD_A_GREEN             RGBto565(0x43, 0xC8, 0x00)
#define LCD_A_BLUE              RGBto565(0x07, 0xA8, 0xE0) 

#define LCD_A_PURPLE            RGBto565(0xBB, 0x36, 0xFF)
#define LCD_A_ORANGE            RGBto565(0xF9, 0x56, 0x1D)
#define LCD_WHITE               0xFFFF

void video_init(void);
void video_update(uint16_t address);
void video_scan_line_set(uint16_t line);
void video_buffer_clear(void);
void video_buffer_get(uint16_t *buf);
uint16_t video_address_get(void);
void video_line_data_get(uint8_t *video_line_data);

uint32_t lcd_render_line(void);
uint32_t lcd_render(void);

#endif /* __VIDEO_H__ */
