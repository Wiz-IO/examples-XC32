#ifndef _HAL_LCD_H_
#define _HAL_LCD_H_
#ifdef __cplusplus
extern "C"
{
#endif

#include <user_config.h>

///////////////////////////////////////////////////////////////////////////////
// USER EDIT //////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifndef USER_LCD            // ILI9341

#define LCD_WIDTH           (240u)
#define LCD_HEIGHT          (320u)

#define LCD_R_GRP           D /* LCD RESET */
#define LCD_R_PIN           4

#define LCD_D_GRP           D /* LCD D/C */
#define LCD_D_PIN           14

#define LCD_SPI             1 /* SPIx */
#define LCD_SPI_BRG_VAL     0 /* 50 MHz = 38.46 fps */

extern                      void spi_set_pins(void);
#define LCD_SPI_SET_PINS()  spi_set_pins()

extern                      void delay(uint32_t);
#define LCD_DELAY           delay

extern                      const uint8_t ILI9341_commands[];
#define LCD_CONFIG          ILI9341_commands

#endif
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#include <stdint.h>

#undef  CONCAT_EX
#undef  CONCAT
#define CONCAT_EX(X, Y, Z)  X##Y##Z
#define CONCAT(X, Y, Z)     CONCAT_EX(X, Y, Z)

#define LCD_RST_INIT()      CONCAT(TRIS, LCD_R_GRP, CLR) = 1 << LCD_R_PIN
#define LCD_RST_0()         CONCAT(LAT,  LCD_R_GRP, CLR) = 1 << LCD_R_PIN
#define LCD_RST_1()         CONCAT(LAT,  LCD_R_GRP, SET) = 1 << LCD_R_PIN

#define LCD_DC_INIT()       CONCAT(TRIS, LCD_D_GRP, CLR) = 1 << LCD_D_PIN
#define LCD_DC_CMD()        CONCAT(LAT,  LCD_D_GRP, CLR) = 1 << LCD_D_PIN
#define LCD_DC_DATA()       CONCAT(LAT,  LCD_D_GRP, SET) = 1 << LCD_D_PIN

#define LCD_SPI_CON         CONCAT( SPI, LCD_SPI,  CON)
#define LCD_SPI_STA         CONCAT( SPI, LCD_SPI,  STAT)
#define LCD_SPI_BRG         CONCAT( SPI, LCD_SPI,  BRG)
#define LCD_SPI_BUF         CONCAT( SPI, LCD_SPI,  BUF)
#define LCD_SPI_VECTOR      CONCAT(_SPI, LCD_SPI, _TX_VECTOR)

void lcd_init(int rot);
void lcd_setRotation(uint8_t rot);
void lcd_setAddrWindow(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye);
void lcd_fillScreen(uint16_t color);
void lcd_fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void lcd_drawImage(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *buffer);

#ifdef __cplusplus
}
#endif
#endif /* _HAL_LCD_H_ */