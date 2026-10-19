/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef AW88399_COMPAT_H
#define AW88399_COMPAT_H

#include <IOKit/IOLib.h>
#include <IOKit/IOLocks.h>
#include <libkern/libkern.h>
#include <sys/errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef uint16_t __be16;
typedef uint16_t __le16;
typedef uint32_t __le32;

#ifndef __counted_by
#define __counted_by(member)
#endif
#define GFP_KERNEL 0
#define EXPORT_SYMBOL_GPL(symbol)
#define MODULE_DESCRIPTION(description)
#define MODULE_LICENSE(license)
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
#define BIT(bit) (1U << (bit))
#ifdef min
#undef min
#endif
#define min(left, right) ((left) < (right) ? (left) : (right))
#ifdef max
#undef max
#endif
#define max(left, right) ((left) > (right) ? (left) : (right))
#define container_of(pointer, type, member) \
    ((type *)((char *)(pointer) - offsetof(type, member)))
#define IS_ERR(pointer) ((pointer) == NULL)
#define IS_ERR_OR_NULL(pointer) ((pointer) == NULL)
#define kzalloc_obj(object) aw_kzalloc(sizeof(object))
#define guard(type) aw_guard_mutex
#define __free(cleanup_fn) __attribute__((cleanup(aw_auto_free)))
#define DECLARE_CRC8_TABLE(name) static u8 name[256]

struct aw_managed_allocation;

struct device {
    struct aw_managed_allocation *managed_allocations;
    void *of_node;
};

struct device_node;

struct i2c_adapter {
    int nr;
};

struct i2c_client {
    unsigned short addr;
    struct i2c_adapter *adapter;
    struct device dev;
};

struct gpio_desc {
    int unused;
};

struct mutex {
    int unused;
};

struct work_struct {
    int unused;
};

struct delayed_work {
    struct work_struct work;
};

enum regmap_endian {
    REGMAP_ENDIAN_DEFAULT,
    REGMAP_ENDIAN_LITTLE,
    REGMAP_ENDIAN_BIG
};

struct regmap_config {
    unsigned int reg_bits;
    unsigned int val_bits;
    unsigned int max_register;
    enum regmap_endian reg_format_endian;
    enum regmap_endian val_format_endian;
};

struct regmap {
    void *context;
    int (*read)(void *context, u8 reg, u16 *value);
    int (*write)(void *context, u8 reg, u16 value);
    int (*raw_write)(void *context, u8 reg, const u8 *data, u16 length);
};

#ifdef __cplusplus
extern "C" {
#endif

void *aw_kzalloc(size_t size);
void aw_kfree(void *pointer);
void aw_auto_free(void *pointer_address);
void *aw_devm_kzalloc(struct device *device, size_t size);
void *aw_devm_kcalloc(struct device *device, size_t count, size_t size);
void aw_devm_kfree(struct device *device, void *pointer);
void aw_devm_release(struct device *device);
void aw_guard_mutex(struct mutex *mutex);
void aw_usleep_range(unsigned int minimum_us, unsigned int maximum_us);
void aw_swab16_array(u16 *values, size_t count);
u32 aw_crc32c(u32 initial, const void *data, size_t length);

static inline u32 aw_get_le32(const void *pointer) {
    const u8 *bytes = (const u8 *)pointer;
    return (u32)bytes[0] | ((u32)bytes[1] << 8) |
           ((u32)bytes[2] << 16) | ((u32)bytes[3] << 24);
}

#define kzalloc(size, flags) aw_kzalloc(size)
#define kfree(pointer) aw_kfree(pointer)
#define devm_kzalloc(device, size, flags) aw_devm_kzalloc(device, size)
#define devm_kcalloc(device, count, size, flags) aw_devm_kcalloc(device, count, size)
#define devm_kfree(device, pointer) aw_devm_kfree(device, pointer)
#define mutex_init(mutex) ((void)(mutex))
#define mutex_lock(mutex) ((void)(mutex))
#define mutex_unlock(mutex) ((void)(mutex))
#define usleep_range(minimum, maximum) aw_usleep_range(minimum, maximum)
#define gpiod_set_value_cansleep(gpio, value) ((void)(gpio), (void)(value))
#define queue_delayed_work(workqueue, work, delay) \
    ((void)(workqueue), (void)(work), (void)(delay), 0)
#define system_dfl_wq ((void *)0)

#define dev_err(device, format, ...) \
    IOLog("AW88399: " format "\n", ##__VA_ARGS__)
#define dev_warn(device, format, ...) \
    IOLog("AW88399: " format "\n", ##__VA_ARGS__)
#define dev_info(device, format, ...) \
    IOLog("AW88399: " format "\n", ##__VA_ARGS__)
#define dev_dbg(device, format, ...) ((void)0)
#define dev_err_probe(device, error, format, ...) (error)
#define pr_err(format, ...) IOLog("AW88399: " format "\n", ##__VA_ARGS__)

int regmap_read(struct regmap *map, unsigned int reg, unsigned int *value);
int regmap_write(struct regmap *map, unsigned int reg, unsigned int value);
int regmap_update_bits(struct regmap *map, unsigned int reg,
                       unsigned int mask, unsigned int value);
int regmap_raw_write(struct regmap *map, unsigned int reg,
                     const void *data, size_t length);

void crc8_populate_lsb(u8 table[256], u8 polynomial);
u8 crc8(const u8 table[256], const u8 *data, size_t length, u8 initial);
u32 crc32_le(u32 initial, const u8 *data, size_t length);
int of_property_read_u32(const struct device_node *node, const char *name,
                         u32 *value);

#ifdef __cplusplus
}
#endif

static inline u16 cpu_to_be16p(const u16 *value) {
    return (u16)((*value << 8) | (*value >> 8));
}

static inline u16 be16_to_cpup(const u16 *value) {
    return cpu_to_be16p(value);
}

#define cpu_to_le16(value) ((u16)(value))
#define cpu_to_le32(value) ((u32)(value))
#define le32_to_cpup(value) aw_get_le32(value)
#define swab16_array(values, count) aw_swab16_array((u16 *)(values), (count))
#define crc32c(initial, data, length) aw_crc32c((initial), (data), (length))

#endif
