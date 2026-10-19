/* SPDX-License-Identifier: GPL-2.0-only */
#include "AW88399I2CProtocol.h"

int aw88399_read_register(const aw88399_i2c_transport *transport,
                          uint8_t reg,
                          uint16_t *value) {
    uint8_t address[1] = {reg};
    uint8_t data[2];
    int result;

    if (transport == 0 || transport->write_read == 0 || value == 0)
        return -1;

    result = transport->write_read(transport->context, address, 1, data, 2);
    if (result != 0)
        return result;

    *value = (uint16_t)(((uint16_t)data[0] << 8) | data[1]);
    return 0;
}

int aw88399_write_register(const aw88399_i2c_transport *transport,
                           uint8_t reg,
                           uint16_t value) {
    uint8_t message[3] = {reg, (uint8_t)(value >> 8), (uint8_t)value};

    if (transport == 0 || transport->write == 0)
        return -1;

    return transport->write(transport->context, message, 3);
}

int aw88399_probe_read_only(const aw88399_i2c_transport *transport,
                            uint16_t *chip_id) {
    int result;

    if (chip_id == 0)
        return -1;

    result = aw88399_read_register(transport, AW88399_REG_ID, chip_id);
    if (result != 0)
        return result;

    return *chip_id == AW88399_EXPECTED_CHIP_ID ? 0 : -2;
}
