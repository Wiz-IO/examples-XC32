#ifndef FLASH_API_H
#define FLASH_API_H

#include <stdint.h>
#include <stdbool.h>

void board_flash_init(void);
uint32_t board_flash_size(void);
void board_flash_read(uint32_t addr, void *buffer, uint32_t len);
void board_flash_write(uint32_t address, void const *data, uint32_t len);
void board_flash_erase_app(void);
void board_flash_flush(void);
bool board_flash_protect_bootloader(bool protect);

#endif