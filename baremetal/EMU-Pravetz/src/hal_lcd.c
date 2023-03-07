#include <hal_lcd.h>
#include <xc.h>
#include <sys/kmem.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef LCD_USE_DMA
#include <hal_dma.h>
#include <hal_irq.h>
#endif

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

#ifndef LCD_BUFFER_SIZE
#define LCD_BUFFER_SIZE (2)
#endif
#if LCD_BUFFER_SIZE < 2
#error LCD_BUFFER_SIZE must be minimum 2
#endif
static uint16_t __attribute__((coherent, aligned(32))) dma_buffer[LCD_BUFFER_SIZE];

static DMA_INST_T *TX = NULL;

void tx_cb(void *param)
{
    irq_setIE(TX->irq, false);
}

static void SPI_DMA_INIT(void)
{
    TX = DMA_Channel(1, DMA_OPEN_DEFAULT, tx_cb);
    if (TX)
    {
        DMA_SrcDst(TX->dma, dma_buffer, 2, &LCD_SPI_BUF, 2, 2);
        TX->dma->econ.CHSIRQ = LCD_SPI_VECTOR;
        TX->dma->econ.SIRQEN = 1;
    }
    else
    {
        PRINTF("[ERROR] SPI_DMA_INIT\n");
    }
}

static void SPI_DMA_START(void)
{
    DMA_ChannelEnable(TX->dma, false, 0);
    irq_setIF(LCD_SPI_VECTOR, false);
    irq_setIF(TX->irq, false);
    irq_setIE(TX->irq, true);
    DMA_ChannelEnable(TX->dma, true, _DCH0INT_CHBCIE_MASK);
    while (irq_getIE(TX->irq))
        continue;
}

static size_t SPI_DMA_FILL(uint32_t data, uint32_t data_size, size_t size_bytes)
{
    if (size_bytes)
    {
        if (size_bytes > 65536u)
            size_bytes = 65536u;
        TX->dma->ssiz = size_bytes;
        uint16_t *p = dma_buffer;
        uint32_t s = size_bytes / data_size;
        while (s--)
            *p++ = data;
        TX->dma->ssa = KVA_TO_PA(dma_buffer);
        SPI_DMA_START();
    }
    else
    {
        PRINTF("[ERROR] SPI_DMA_FILL\n");
    }
    return size_bytes;
}

static size_t SPI_DMA_DATA(const void *buffer_bytes, size_t size_bytes)
{
    if (size_bytes && buffer_bytes)
    {
        if (size_bytes > 65536u)
            size_bytes = 65536u;
        TX->dma->ssiz = size_bytes;
        if (IS_KVA01(buffer_bytes)) // IS RAM
        {
            if (IS_KVA0(buffer_bytes))
            {
                TX->dma->ssa = KVA_TO_PA(KVA0_TO_KVA1(buffer_bytes));
            }
            else
            {
                TX->dma->ssa = KVA_TO_PA(buffer_bytes);
            }
        }
        else // IS FLASH
        {
            PRINTF("[ERROR] TODO: FROM FLASH\n");
        }
        SPI_DMA_START();
    }
    else
    {
        PRINTF("[ERROR] SPI_DMA_DATA\n");
    }
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
    SPI_DMA_INIT();
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

void lcd_setAddrWindow(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye)
{
    lcd_write_cmd(0x2A); // ILI9341_CASET
#if LCD_USE_DMA
    LCD_DC_DATA();
    spi_mode(SPI_WORD);
    dma_buffer[0] = xs;
    dma_buffer[1] = xe;
    SPI_DMA_DATA(dma_buffer, sizeof(uint32_t));
#else
    lcd_write_data(xs, SPI_WORD);
    lcd_write_data(xe, SPI_WORD);
#endif
    lcd_write_cmd(0x2B); // ILI9341_PASET
#if LCD_USE_DMA
    LCD_DC_DATA();
    spi_mode(SPI_WORD);
    dma_buffer[0] = ys;
    dma_buffer[1] = ye;
    SPI_DMA_DATA(dma_buffer, sizeof(uint32_t));
#else
    lcd_write_data(ys, SPI_WORD);
    lcd_write_data(ye, SPI_WORD);
#endif
    lcd_write_cmd(0x2C); // ILI9341_RAMWR
    LCD_DC_DATA();
}

void lcd_setRotation(uint8_t rot)
{
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
    // if (x + w - 1 >= lcd_width) w = lcd_width - x;
    // if (y + h - 1 >= lcd_height) h = lcd_height - y;
    lcd_setAddrWindow(x, y, x + w - 1, y + h - 1);
    size_t size = (size_t)w * (size_t)h;
#ifndef LCD_USE_DMA
    while (size--)
        lcd_write_data(color, SPI_WORD);
#else
    spi_mode(SPI_WORD);
    while (size)
        size -= SPI_DMA_FILL(color, sizeof(short), size * sizeof(short)) / sizeof(short);
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
    spi_mode(SPI_WORD);
    size_t sent;
    while (size)
    {
        sent = SPI_DMA_DATA(buffer, size << 1) >> 1;
        size -= sent;
        buffer += sent;
    }
#endif
}

void lcd_commands_init(const uint8_t *address)
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
    if (rot > -1)
        lcd_setRotation(rot);
}

// EOF ////////////////////////////////////////////////////////////////////////