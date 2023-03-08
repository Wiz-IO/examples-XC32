#include <hal_lcd.h>
#include <xc.h>
#include <sys/kmem.h>
#include <stdbool.h>
#include <stdio.h>
#include <hal_irq.h>
#include <sys_devcon.h>

#undef PRINTF
#define PRINTF printf

///////////////////////////////////////////////////////////////////////////////

uint16_t lcd_width = LCD_WIDTH;
uint16_t lcd_height = LCD_HEIGHT;

#define CMD_DELAY 0x80
const uint8_t ILI9341_commands[] = {
    0x16, 0x01, 0x80, 0x05, 0xEF, 0x03, 0x03, 0x80, 0x02, 0xCF, 0x03, 0x00, 0xC1, 0x30, 0xED, 0x04, 0x64, 0x03, 0x12, 0x81, 0xE8, 0x03, 0x85, 0x00, 0x78, 0xCB, 0x05, 0x39, 0x2C, 0x00, 0x34, 0x02, 0xF7, 0x01, 0x20, 0xEA, 0x02, 0x00, 0x00, 0xC0, 0x01, 0x23, 0xC1, 0x01, 0x10, 0xC5, 0x02, 0x3E, 0x28, 0xC7, 0x01, 0x86, 0x36, 0x01, 0x48, 0x3A, 0x01, 0x55, 0xB1, 0x02, 0x00, 0x18, 0xB6, 0x03, 0x08, 0x82, 0x27, 0xF2, 0x01, 0x00, 0x26, 0x01, 0x01, 0xE0, 0x0F, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00, 0xE1, 0x0F, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F, 0x11, 0x80, 0x78, 0x29, 0x00};

///////////////////////////////////////////////////////////////////////////////

#ifdef LCD_USE_DMA

#undef DMA_MAX_SIZE
#define DMA_MAX_SIZE (65536u)

static uint16_t __attribute__((aligned(4))) dma_buffer[2]; // coherent

static uint32_t translate_address(const void *address)
{
    uint32_t A = (uint32_t)address;

    /* Set the source address */
    /* Check if the address lies in the KSEG2 for MZ devices */
    if ((A >> 29) == 0x6)
    {
        if ((A >> 28) == 0xc)
        {
            // EBI Address translation
            A = ((A | 0x20000000) & 0x2FFFFFFF);
        }
        else if ((A >> 28) == 0xD)
        {
            // SQI Address translation
            A = ((A | 0x30000000) & 0x3FFFFFFF);
        }
    }
    else if ((A >> 29) == 0x7)
    {
        if ((A >> 28) == 0xE)
        {
            // EBI Address translation
            A = ((A | 0x20000000) & 0x2FFFFFFF);
        }
        else if ((A >> 28) == 0xF)
        {
            // SQI Address translation
            A = ((A | 0x30000000) & 0x3FFFFFFF);
        }
    }
    else
    {
        /* KSEG0 / KSEG1 */
        if (IS_KVA0(A))
            A = KVA_TO_PA(KVA0_TO_KVA1(A));
        else
            A = KVA_TO_PA(A);
    }
    return A;
}

static inline void DMA_ChannelEnable(int dma_intr_mask)
{
    IRQ_Clear(_DMA0_VECTOR);
    IRQ_Enable(_DMA0_VECTOR);
    DCH0INT = dma_intr_mask;
    DCH0CONSET = _DCH0CON_CHEN_MASK;
}

static inline void DMA_ChannelDisable()
{
    IRQ_Disable(_DMA0_VECTOR);
    IRQ_Clear(_DMA0_VECTOR);
    DCH0CONCLR = _DCH0CON_CHEN_MASK;
    DCH0INTCLR = -1;
}

static inline void DMA_Init(void)
{
    IRQ_Disable(_DMA0_VECTOR);
    IRQ_Clear(_DMA0_VECTOR);
    IRQ_Priority(_DMA0_VECTOR, 4, 0); // IPL4AUTO

    DCH0CON = 1; // DMA_PRIO
    DCH0ECONCLR = -1;
    DCH0INTCLR = -1;

    DCH0ECONbits.CHSIRQ = _SPI1_TX_VECTOR;
    DCH0ECONbits.SIRQEN = 1;

    DCH0DSA = KVA_TO_PA(&LCD_SPI_BUF);
    DCH0DSIZ = 2;
    DCH0CSIZ = 2;

    DMACONSET = 0x8000; // enale DMA
}

void __attribute__((vector(_DMA0_VECTOR), interrupt(IPL4AUTO), nomips16)) DMA_0_Handler()
{
    IRQ_Disable(_DMA0_VECTOR);
}

// LCD DMA ////////////////////////////////////////////////////////////////////

static size_t SPI_DMA_FILL(uint16_t color, size_t size_bytes)
{
    if (size_bytes > sizeof(dma_buffer))
        size_bytes = sizeof(dma_buffer);
    for (int i = 0; i < size_bytes / 2; i++)
        dma_buffer[i] = color;
    DMA_ChannelDisable();
    DCH0SSA = translate_address(dma_buffer);
    DCH0SSIZ = size_bytes;
    SYS_DEVCON_DataCacheClean((uint32_t)dma_buffer, sizeof(dma_buffer));
    DMA_ChannelEnable(_DCH0INT_CHBCIE_MASK);
    while (IRQ_IsEnabled(_DMA0_VECTOR))
        continue;
    SYS_DEVCON_DataCacheInvalidate((uint32_t)dma_buffer, sizeof(dma_buffer));
    return size_bytes;
}

static size_t SPI_DMA_DATA(const void *buffer_bytes, size_t size_bytes)
{
    if (size_bytes > DMA_MAX_SIZE)
        size_bytes = DMA_MAX_SIZE;
    DMA_ChannelDisable();
    DCH0SSA = translate_address(buffer_bytes);
    DCH0SSIZ = size_bytes;
    SYS_DEVCON_DataCacheClean((uint32_t)buffer_bytes, size_bytes);
    DMA_ChannelEnable(_DCH0INT_CHBCIE_MASK);
    while (IRQ_IsEnabled(_DMA0_VECTOR))
        continue;
    SYS_DEVCON_DataCacheInvalidate((uint32_t)buffer_bytes, size_bytes);
    return size_bytes;
}

#endif // LCD_USE_DMA

// SPI ////////////////////////////////////////////////////////////////////////

#define SPI_BYTE (0)
#define SPI_WORD (1)

static inline void spi_mode(bool mode)
{
    while (LCD_SPI_STA & _SPI1STAT_SPIBUSY_MASK) // MUST
        continue;
    LCD_SPI_CON &= ~_SPI1CON_ON_MASK; // MUST
    if (mode == SPI_WORD)
        LCD_SPI_CON |= _SPI1CON_MODE16_MASK;
    else
        LCD_SPI_CON &= ~_SPI1CON_MODE16_MASK;
    LCD_SPI_CON |= _SPI1CON_ON_MASK;
}

static uint16_t spi_send(uint16_t tx)
{
    LCD_SPI_BUF = tx;
    while (LCD_SPI_STA & _SPI1STAT_SPIRBE_MASK)
        continue;
    return LCD_SPI_BUF;
}

static void spi_init(void)
{
#ifdef LCD_SPI_SET_PINS
    LCD_SPI_SET_PINS();
#endif
    LCD_SPI_CON = 0;
    LCD_SPI_BRG = LCD_SPI_BRG_VAL;
    LCD_SPI_CON = 0x00018265;
#ifdef LCD_USE_DMA
    DMA_Init();
#endif
}

// LCD ////////////////////////////////////////////////////////////////////////

void lcd_write_cmd(uint8_t c)
{
    LCD_DC_CMD();
    spi_mode(SPI_BYTE);
    spi_send(c);
}

void lcd_write_data(uint16_t d, bool mode)
{
    LCD_DC_DATA();
    spi_mode(mode);
    spi_send(d);
}

uint16_t lcd_get_width() { return lcd_width; }
uint16_t lcd_get_height() { return lcd_height; }

void lcd_setAddrWindow(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye)
{
    lcd_write_cmd(0x2A); // ILI9341_CASET

#ifdef LCD_USE_DMA
    LCD_DC_DATA();
    spi_mode(SPI_WORD);
    dma_buffer[0] = xs;
    dma_buffer[1] = xe;
    SPI_DMA_DATA(dma_buffer, 4);
#else
    lcd_write_data(xs, SPI_WORD);
    lcd_write_data(xe, SPI_WORD);
#endif
    lcd_write_cmd(0x2B); // ILI9341_PASET

#ifdef LCD_USE_DMA
    LCD_DC_DATA();
    spi_mode(SPI_WORD);
    dma_buffer[0] = ys;
    dma_buffer[1] = ye;
    SPI_DMA_DATA(dma_buffer, 4);
#else
    lcd_write_data(ys, SPI_WORD);
    lcd_write_data(ye, SPI_WORD);
#endif

    lcd_write_cmd(0x2C); // ILI9341_RAMWR
    LCD_DC_DATA();
}

void lcd_setRotation(uint8_t rot)
{
    if (rot < 4)
        lcd_write_cmd(0x36); // ILI9341_MADCTL
    switch (rot & 3)
    {
    case 0:
        lcd_write_data(0x40 | 0x08, SPI_BYTE); // ILI9341_MADCTL_MX
        lcd_width = LCD_WIDTH;
        lcd_height = LCD_HEIGHT;
        break;
    case 1:
        lcd_write_data(0x20 | 0x08, SPI_BYTE); // ILI9341_MADCTL_MV
        lcd_width = LCD_HEIGHT;
        lcd_height = LCD_WIDTH;
        break;
    case 2:
        lcd_write_data(0x80 | 0x08, SPI_BYTE); // ILI9341_MADCTL_MY
        lcd_width = LCD_WIDTH;
        lcd_height = LCD_HEIGHT;
        break;
    case 3:
        lcd_write_data(0x40 | 0x80 | 0x20 | 0x08, SPI_BYTE);
        lcd_width = LCD_HEIGHT;
        lcd_height = LCD_WIDTH;
        break;
    }
}

void lcd_fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    if (x >= lcd_width || y >= lcd_height || w <= 0 || h <= 0)
        return;
    lcd_setAddrWindow(x, y, x + w - 1, y + h - 1);
    size_t size = (size_t)w * (size_t)h;
#ifndef LCD_USE_DMA
    while (size--)
        lcd_write_data(color, SPI_WORD);
#else
    spi_mode(SPI_WORD);
    while (size)
        size -= SPI_DMA_FILL(color, size << 1) >> 1;
#endif
}

void lcd_fillScreen(uint16_t color) { lcd_fillRect(0, 0, lcd_width, lcd_height, color); }

void lcd_drawImage(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *buffer)
{
    if (x >= lcd_width || y >= lcd_height || w <= 0 || h <= 0 || buffer == NULL)
        return;
    lcd_setAddrWindow(x, y, x + w - 1, y + h - 1);
    size_t size = (size_t)w * (size_t)h;
#ifndef LCD_USE_DMA
    while (size--) // slow
        lcd_write_data(*buffer++, SPI_WORD);
#else
    size_t sent;
    spi_mode(SPI_WORD);
    while (size)
    {
        sent = SPI_DMA_DATA(buffer, size << 1) >> 1;
        size -= sent;
        buffer += sent;
    }
#endif
}

static void lcd_commands_init(const uint8_t *address)
{
    uint8_t *addr = (uint8_t *)address;
    uint8_t numCommands, numArgs;
    uint16_t ms;
    numCommands = *addr++;
    while (numCommands--)
    {
        lcd_write_cmd(*addr++);
        numArgs = *addr++;
        ms = numArgs & 0x80;
        numArgs &= 0x7F;
        while (numArgs--)
            lcd_write_data(*addr++, SPI_BYTE);
        if (ms)
        {
            ms = *addr++;
            if (ms == 255)
                ms = 500;
            LCD_DELAY(ms);
        }
    }
}

void lcd_init(int rot)
{
    spi_init();
    LCD_RST_INIT();
    LCD_DC_INIT();
    LCD_RST_1();
    LCD_DELAY(5);
    LCD_RST_0();
    LCD_DELAY(20);
    LCD_RST_1();
    LCD_DELAY(150);
    lcd_commands_init(LCD_CONFIG);
    lcd_setRotation(rot);
}

///////////////////////////////////////////////////////////////////////////////

void lcd_draw_scan_line(uint16_t *line_data, uint16_t scanline, int max_x, int max_y)
{
    if (scanline < lcd_get_height())
    {
        int xoffset = (lcd_width - max_x) / 2;
        int yoffset = (lcd_height - max_y) / 2;
        lcd_drawImage(xoffset, scanline + yoffset, max_x, 1, line_data); // centered 
    }
}
