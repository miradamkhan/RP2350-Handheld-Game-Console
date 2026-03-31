#ifndef EEPROM_H
#define EEPROM_H

#include <stdbool.h>
#include <stdint.h>

#define EEPROM_TOTAL_BYTES   4096u
#define EEPROM_PAGE_SIZE     32u

void eeprom_init(void);
bool eeprom_read(uint16_t mem_addr, uint8_t* dst, uint16_t len);
bool eeprom_write(uint16_t mem_addr, const uint8_t* src, uint16_t len);
bool eeprom_read_u16(uint16_t mem_addr, uint16_t* out);
bool eeprom_write_u16(uint16_t mem_addr, uint16_t value);
void eeprom_test(void);

#endif // EEPROM_H
