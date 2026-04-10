/*
 * eeprom.h — 24AA32AF I2C EEPROM driver (32 Kbit = 4 KB).
 *
 * The 24AA32AF uses a 2-byte address and 32-byte page size.
 * High scores are stored with a 2-byte magic marker for validity.
 *   - Tetris  high score at address 0x0000
 *   - Flappy  high score at address 0x0010
 */

#ifndef EEPROM_H
#define EEPROM_H

#include <stdint.h>
#include <stdbool.h>

/* 7-bit I2C address of the 24AA32AF with A0=A1=A2=0. */
#define EEPROM_I2C_ADDR  0x50

/* EEPROM address slots for each game's high score. */
#define EEPROM_ADDR_TETRIS_HS   0x0000
#define EEPROM_ADDR_FLAPPY_HS   0x0010
#define EEPROM_ADDR_INVADERS_HS 0x0020
#define EEPROM_ADDR_2048_HS     0x0030

/* Initialise I2C bus for the EEPROM. */
void eeprom_init(void);

/* Read `len` bytes starting at `mem_addr` into `dst`.
 * Returns true on success. */
bool eeprom_read(uint16_t mem_addr, uint8_t *dst, uint16_t len);

/* Write `len` bytes from `src` starting at `mem_addr`.
 * Handles page-boundary splitting and write-cycle delays.
 * Returns true on success. */
bool eeprom_write(uint16_t mem_addr, const uint8_t *src, uint16_t len);

/* Read a stored high score from the given EEPROM address.
 * Returns 0 if EEPROM is blank/invalid. */
uint32_t eeprom_read_high_score(uint16_t addr);

/* Write a new high score at the given EEPROM address.
 * Only writes if `score` > stored value. */
void eeprom_write_high_score(uint16_t addr, uint32_t score);

/* Quick read/write round-trip test (prints to stdio). */
void eeprom_test(void);

#endif /* EEPROM_H */
