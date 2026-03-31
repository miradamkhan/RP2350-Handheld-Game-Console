/*
 * eeprom.c — 24AA32AF I2C EEPROM driver for high-score storage.
 *
 * The 24AA32AF is a 32 Kbit (4 KB) EEPROM with:
 *   - 2-byte memory address
 *   - 32-byte page size
 *   - 5 ms max write-cycle time
 *   - 7-bit I2C address 0x50 (A0=A1=A2 tied low)
 *
 * High score layout at address 0x0000:
 *   [0..3] uint32_t score, big-endian
 *   [4..5] 0xBE 0xEF magic marker
 */

#include "eeprom.h"
#include "pins.h"

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"

#include <stdio.h>
#include <string.h>

/* ── Constants ── */

#define PAGE_SIZE       32
#define WRITE_CYCLE_MS   5
#define MAGIC_0       0xBE
#define MAGIC_1       0xEF
#define HS_ADDR       0x0000   /* high-score stored at start of EEPROM */

/* ── Public API ── */

void eeprom_init(void)
{
    i2c_init(I2C_PORT, I2C_BAUD_HZ);
    gpio_set_function(PIN_I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_I2C_SDA);
    gpio_pull_up(PIN_I2C_SCL);
}

bool eeprom_read(uint16_t mem_addr, uint8_t *dst, uint16_t len)
{
    uint8_t addr_buf[2] = {
        (uint8_t)(mem_addr >> 8),
        (uint8_t)(mem_addr)
    };

    int ret = i2c_write_blocking(I2C_PORT, EEPROM_I2C_ADDR,
                                 addr_buf, 2, true);  /* no stop */
    if (ret < 0) return false;

    ret = i2c_read_blocking(I2C_PORT, EEPROM_I2C_ADDR, dst, len, false);
    return ret >= 0;
}

bool eeprom_write(uint16_t mem_addr, const uint8_t *src, uint16_t len)
{
    while (len > 0) {
        /* Bytes remaining in the current 32-byte page. */
        uint16_t page_remain = PAGE_SIZE - (mem_addr % PAGE_SIZE);
        uint16_t chunk = (len < page_remain) ? len : page_remain;

        /* Build [addr_hi, addr_lo, data...] */
        uint8_t buf[2 + PAGE_SIZE];
        buf[0] = (uint8_t)(mem_addr >> 8);
        buf[1] = (uint8_t)(mem_addr);
        memcpy(&buf[2], src, chunk);

        int ret = i2c_write_blocking(I2C_PORT, EEPROM_I2C_ADDR,
                                     buf, 2 + chunk, false);
        if (ret < 0) return false;

        /* Wait for the internal write cycle to complete. */
        sleep_ms(WRITE_CYCLE_MS);

        mem_addr += chunk;
        src      += chunk;
        len      -= chunk;
    }
    return true;
}

uint32_t eeprom_read_high_score(void)
{
    uint8_t buf[6];
    if (!eeprom_read(HS_ADDR, buf, 6)) return 0;

    if (buf[4] != MAGIC_0 || buf[5] != MAGIC_1) return 0;

    uint32_t score = ((uint32_t)buf[0] << 24) |
                     ((uint32_t)buf[1] << 16) |
                     ((uint32_t)buf[2] <<  8) |
                     ((uint32_t)buf[3]);
    return score;
}

void eeprom_write_high_score(uint32_t score)
{
    uint32_t stored = eeprom_read_high_score();
    if (score <= stored) return;   /* only write a new record */

    uint8_t buf[6] = {
        (uint8_t)(score >> 24),
        (uint8_t)(score >> 16),
        (uint8_t)(score >>  8),
        (uint8_t)(score),
        MAGIC_0,
        MAGIC_1
    };
    eeprom_write(HS_ADDR, buf, 6);
}

void eeprom_test(void)
{
    printf("=== EEPROM Test ===\n");

    uint8_t write_val = 0xA5;
    uint16_t test_addr = 0x0100;  /* safe area away from high-score data */

    bool ok = eeprom_write(test_addr, &write_val, 1);
    printf("Write 0x%02X to 0x%04X: %s\n", write_val, test_addr,
           ok ? "OK" : "FAIL");

    uint8_t read_val = 0;
    ok = eeprom_read(test_addr, &read_val, 1);
    printf("Read back: 0x%02X %s\n", read_val,
           (ok && read_val == write_val) ? "MATCH" : "MISMATCH");

    uint32_t hs = eeprom_read_high_score();
    printf("Stored high score: %lu\n", (unsigned long)hs);

    printf("=== EEPROM Test Done ===\n");
}
