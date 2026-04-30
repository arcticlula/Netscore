#pragma once

#include <stdbool.h>
#include <time.h>

#define DS3231_I2C_ADDR 0x68

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Check if the DS3231 is responding on the configured I2C bus.
 * @return true if successful
 */
bool ds3231_init(void);

/**
 * @brief Fetch the current time from the RTC directly into a standard tm struct.
 * @param timeinfo Pointer to a standard C time structure to populate.
 * @return true on valid read.
 */
bool ds3231_get_time(struct tm *timeinfo);

/**
 * @brief Set the RTC time from a populated tm struct.
 * @param timeinfo Pointer to the reference time struct.
 * @return true on valid write.
 */
bool ds3231_set_time(const struct tm *timeinfo);

#ifdef __cplusplus
}
#endif
