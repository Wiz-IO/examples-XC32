#ifndef __VIDEO_H__
#define __VIDEO_H__

#include <stdbool.h>
#include <stdint.h>

#define VIDEO_RESOLUTION_X      280
#define VIDEO_RESOLUTION_Y      192
#define VIDEO_BUFFER_SIZE       (VIDEO_RESOLUTION_X)

#define VIDEO_BYTES_PER_LINE    40
#define SCREEN_LINE_OFFSET      0x80
#define VIDEO_SEGMENT_OFFSET    0x28

#define TEXT_BYTES              8
#define TEXT_BYTES_MASK         7
#define TEXT_LINES              24
#define LORES_PIXEL_WIDTH       14
#define LORES_PIXEL_HEIGHT      4

void video_init(void);
uint8_t video_update_switches(uint16_t address);
uint32_t video_render_line(int line);
uint32_t video_render_screen(void);

#endif /* __VIDEO_H__ */
