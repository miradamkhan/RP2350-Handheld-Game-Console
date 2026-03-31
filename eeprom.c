#include "eeprom.h"

#include <stdio.h>
#include <string.h>

#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include "pico/time.h"

#include "pins.h"

#define EEPROM_WRITE_CYCLE_MS   5

static bool eeprom_wait_write_complete(void) {
    absolute_time_t deadline = delayed_by_ms(get_absolute_time(), 20);
    while (!time_reached(deadline)) {
        int rc = i2c_write_timeout_us(EEPROM_I2C_PORT, EEPROM_I2C_ADDR, NULL, 0, false, 500);
        if (rc >= 0) {
            return true;
        }
    }
    return false;
}

void eeprom_init(void) {
    i2c_init(EEPROM_I2C_PORT, 400 * 1000);
    gpio_set_function(EEPROM_PIN_SDA, GPIO_FUNC_I2C);
    gpio_set_function(EEPROM_PIN_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(EEPROM_PIN_SDA);
    gpio_pull_up(EEPROM_PIN_SCL);
}

bool eeprom_read(uint16_t mem_addr, uint8_t* dst, uint16_t len) {
    if (!dst || len == 0) return false;
    if ((uint32_t)mem_addr + len > EEPROM_TOTAL_BYTES) return false;

    uint8_t addr_buf[2] = {(uint8_t)(mem_addr >> 8), (uint8_t)(mem_addr & 0xFF)};
    if (i2c_write_blocking(EEPROM_I2C_PORT, EEPROM_I2C_ADDR, addr_buf, 2, true) != 2) {
        return false;
    }

    return i2c_read_blocking(EEPROM_I2C_PORT, EEPROM_I2C_ADDR, dst, len, false) == (int)len;
}

bool eeprom_write(uint16_t mem_addr, const uint8_t* src, uint16_t len) {
    if (!src || len == 0) return false;
    if ((uint32_t)mem_addr + len > EEPROM_TOTAL_BYTES) return false;

    uint16_t offset = 0;
    while (offset < len) {
        uint16_t cur_addr = (uint16_t)(mem_addr + offset);
        uint16_t page_off = (uint16_t)(cur_addr % EEPROM_PAGE_SIZE);
        uint16_t page_space = (uint16_t)(EEPROM_PAGE_SIZE - page_off);
        uint16_t chunk = (uint16_t)((len - offset) < page_space ? (len - offset) : page_space);

        uint8_t tx[2 + EEPROM_PAGE_SIZE];
        tx[0] = (uint8_t)(cur_addr >> 8);
        tx[1] = (uint8_t)(cur_addr & 0xFF);
        memcpy(&tx[2], &src[offset], chunk);

        if (i2c_write_blocking(EEPROM_I2C_PORT, EEPROM_I2C_ADDR, tx, (uint16_t)(2 + chunk), false) != (int)(2 + chunk)) {
            return false;
        }

        sleep_ms(EEPROM_WRITE_CYCLE_MS);
        if (!eeprom_wait_write_complete()) {
            return false;
        }

        offset = (uint16_t)(offset + chunk);
    }

    return true;
}

bool eeprom_read_u16(uint16_t mem_addr, uint16_t* out) {
    uint8_t data[2] = {0};
    if (!eeprom_read(mem_addr, data, 2)) {
        return false;
    }

    *out = (uint16_t)((data[0] << 8) | data[1]);
    return true;
}

bool eeprom_write_u16(uint16_t mem_addr, uint16_t value) {
    uint8_t data[2] = {(uint8_t)(value >> 8), (uint8_t)(value & 0xFF)};
    return eeprom_write(mem_addr, data, 2);
}

void eeprom_test(void) {
    const uint16_t test_addr = 0x0010;
    const uint8_t test_data[16] = {
        0xE3, 0x62, 0x24, 0xAA, 0x32, 0xAF, 0x55, 0x99,
        0x12, 0x34, 0x56, 0x78, 0x87, 0x65, 0x43, 0x21
    };
    uint8_t read_back[16] = {0};

    bool w_ok = eeprom_write(test_addr, test_data, sizeof(test_data));
    bool r_ok = eeprom_read(test_addr, read_back, sizeof(read_back));

    bool match = false;
    if (w_ok && r_ok) {
        match = (memcmp(test_data, read_back, sizeof(test_data)) == 0);
    }

    printf("EEPROM test: write=%d read=%d match=%d\n", w_ok ? 1 : 0, r_ok ? 1 : 0, match ? 1 : 0);
}
