/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef AW88399_I2C_PROTOCOL_H
#define AW88399_I2C_PROTOCOL_H

#include <stdint.h>

typedef int (*aw88399_write_read_fn)(void *context,
                                    const uint8_t *write_bytes,
                                    uint16_t write_length,
                                    uint8_t *read_bytes,
                                    uint16_t read_length);

typedef int (*aw88399_write_fn)(void *context,
                                const uint8_t *bytes,
                                uint16_t length);

typedef struct {
    void *context;
    aw88399_write_read_fn write_read;
    aw88399_write_fn write;
} aw88399_i2c_transport;

#ifdef __cplusplus
extern "C" {
#endif

enum {
    AW88399_REG_ID = 0x00,
    AW88399_EXPECTED_CHIP_ID = 0x2183
};

int aw88399_read_register(const aw88399_i2c_transport *transport,
                          uint8_t reg,
                          uint16_t *value);
int aw88399_write_register(const aw88399_i2c_transport *transport,
                           uint8_t reg,
                           uint16_t value);
int aw88399_probe_read_only(const aw88399_i2c_transport *transport,
                            uint16_t *chip_id);

#ifdef __cplusplus
}
#endif

#endif
