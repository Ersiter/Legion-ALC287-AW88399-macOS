/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef VOODOO_I2C_DEVICE_NUB_API_HPP
#define VOODOO_I2C_DEVICE_NUB_API_HPP

#include <IOKit/IOService.h>

class VoodooI2CDeviceNub : public IOService {
    OSDeclareDefaultStructors(VoodooI2CDeviceNub)

public:
    IOReturn writeI2C(UInt8 *values, UInt16 length);
    IOReturn writeReadI2C(UInt8 *writeBuffer, UInt16 writeLength,
                          UInt8 *readBuffer, UInt16 readLength);
};

#endif
