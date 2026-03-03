#ifndef NVS_STORE_H
#define NVS_STORE_H

#include "esp_bt_defs.h"
#include <stdbool.h>

#define NVS_NAMESPACE_DEVICES "devices"
#define NVS_MAX_DEVICES 10

typedef struct {
    uint8_t bda[6];
} known_device_t;

bool nvs_load_known_devices(known_device_t *out_list, int *out_count);
bool nvs_add_known_device(const esp_bd_addr_t bda);
bool nvs_remove_known_device(const esp_bd_addr_t bda);

#endif // NVS_STORE_H