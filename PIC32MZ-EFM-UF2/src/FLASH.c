#include "config.h"
#include <string.h>
#include "NVM.h"
#include "FLASH.h"

extern char USB_EJECTED;

// void dump(uint32_t *buf, int len);

// Get size of flash
uint32_t board_flash_size(void) { return NVM_FLASH_SIZE; }

// Read from flash
void board_flash_read(uint32_t addr, void *buffer, uint32_t len)
{
#ifdef APP_DOWNLOAD_ENABLE
    memcpy(buffer, (void *)addr, len);
#else
    memset(buffer, 0, len);
#endif
}

// Write to flash
void board_flash_write(uint32_t address, void const *data, uint32_t len)
{
    if (address > APP_START_ADDRESS + NVM_FLASH_SIZE)
    {
        // PRINT("[NVM] PROTECT: %08X\n", address);
        USB_EJECTED = 2;
        return;
    }

    if (address % NVM_FLASH_PAGESIZE == 0)
    {
        PRINT("[NVM] ERASE: %08X\n", address);
        NVM_PageErase(address);
        while (NVM_IsBusy())
            continue;
    }

    // PRINT("[NVM] WRITE: %08X [ %u ]\n", address, len);
    uint32_t *d = (uint32_t *)data;
    for (int i = 0; i < len; i += 16, d += 4)
    {
        NVM_QuadWordWrite(d, address + i);
        while (NVM_IsBusy())
            continue;
    }
}

// Flush/Sync flash contents
void board_flash_flush(void)
{
    PRINT("[NVM] EOF\n");
    extern char USB_EJECTED;
    USB_EJECTED = 3;
    return;
}
