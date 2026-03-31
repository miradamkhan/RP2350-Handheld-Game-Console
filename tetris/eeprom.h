/*
 * eeprom.h — 24AA32AF I2C EEPROM driver (32 Kbit = 4 KB).
 *
 * The 24AA32AF uses a 2-byte address and 32-byte page size.
 * We store the high score as a 4-byte big-endian uint32 at address 0x0000,
 * with a 2-byte magic marker at 0x0004 for validity.
 */

#ifndef EEPROM_H
#define EEPROM_H

#include <stdint.h>
#include <stdbool.h>

/* 7-bit I2C address of the 24AA32AF with A0=A1=A2=0. */
#define EEPROM_I2C_ADDR  0x50

/* Initialise I2C bus for the EEPROM. */
void eeprom_init(void);

/* Read `len` bytes starting at `mem_addr` into `dst`.
 * Returns true on success. */
bool eeprom_read(uint16_t mem_addr, uint8_t *dst, uint16_t len);

/* Write `len` bytes from `src` starting at `mem_addr`.
 * Handles page-boundary splitting and write-cycle delays.
 * Returns true on success. */
bool eeprom_write(uint16_t mem_addr, const uint8_t *src, uint16_t len);

/* Read the stored high score.  Returns 0 if EEPROM is blank/invalid. */
uint32_t eeprom_read_high_score(void);

/* Write a new high score.  Only writes if `score` > stored value. */
void eeprom_write_high_score(uint32_t score);

/* Quick read/write round-trip test (prints to stdio). */
void eeprom_test(void);

#endif /* EEPROM_H */
