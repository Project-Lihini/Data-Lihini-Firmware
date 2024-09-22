/*
 * MIT License
 * Copyright (c) 2022 Brian T. Park
 */

/**
 * @file epoch.h
 *
 * Function related to configuring the "current epoch" of the library.
 * Uses the algorithm described in
 * https://howardhinnant.github.io/date_algorithms.html.
 *
 * These are intended to be internal implementation details, not normally needed
 * by client applications. The API is not guaranteed to be stable.
 */

#ifndef ACE_TIME_C_EPOCH_H
#define ACE_TIME_C_EPOCH_H

#include <stdbool.h>
#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The epoch year which will be used to interpret the epoch seconds. By default,
 * the current epoch year is 2050, which means that the epoch is
 * 2050-01-01T00:00:00, and the largest date that can be represented by an
 * `int32_t` epoch_seconds is 2118-01-20T03:14:07. To represents dates after
 * this, we would have to change the current epoch year. For example, changing
 * the current epoch year to 2100 allows the epoch_seconds to extend to
 * 2168-01-20T03:14:07.
 */
extern int16_t atc_current_epoch_year;

/** Number of days from internal epoch to the current epoch. */
extern int32_t atc_days_to_current_epoch_from_internal_epoch;

/** Get the current epoch year. */
int16_t atc_get_current_epoch_year(void);

/**
 * Set the current epoch year. Any cached values (e.g. any internal or external
 * evaluations of `atc_time_t`) that used a previous epoch year must be
 * invalidated. Cache invalidation is done automatically by `AtcZoneProcessor`,
 * which takes care of `AtcZonedDateTime`, `AtcTimeZone`, and `AtcZonedExtra`.
 * Any additional application-level caches must be invalidated manually.
 */
void atc_set_current_epoch_year(int16_t year);

/** Convert epoch seconds to the unix seconds from 1970. */
int64_t atc_unix_seconds_from_epoch_seconds(atc_time_t epoch_seconds);

/** Convert the 64-bit unix seconds from 1970 to acetimec epoch seconds. */
atc_time_t atc_epoch_seconds_from_unix_seconds(int64_t unix_seconds);

/** Convert epoch days to unix days. */
int32_t atc_unix_days_from_epoch_days(int32_t epoch_days);

/** Convert unix days to epoch days. */
int32_t atc_epoch_days_from_unix_days(int32_t unix_days);

/**
 * The smallest year (inclusive) for which calculations involving the 32-bit
 * `epoch_seconds` and time zone transitions are guaranteed to be valid without
 * underflowing or overflowing. Valid years satisfy the condition `year >=
 * atc_epoch_valid_year_lower()`.
 *
 * A 32-bit integer has a range of about 136 years, so the half interval is 68
 * years. But the algorithms to calculate transitions in `zone_processor.h` use
 * a 3-year window straddling the current year, so the actual lower limit is
 * probably closer to `atc_get_current_epoch_year() - 66`. To be conservative,
 * this function returns `atc_get_current_epoch_year() - 50`. It may return a
 * smaller value in the future if the internal calculations can be verified to
 * avoid underflow or overflow problems.
 */
int16_t atc_epoch_valid_year_lower(void);

/**
 * The largest year (exclusive) for which calculations involving the 32-bit
 * `epoch_seconds` and time zone transitions are guaranteed to be valid without
 * underflowing or overflowing. Valid years satisfy the condition `year <
 * atc_epoch_valid_year_upper()`.
 *
 * A 32-bit integer has a range of about 136 years, so the half interval is 68
 * years. But the algorithms to calculate the transitions in `zone_processor.h`
 * use a 3-year window straddling the current year, so actual upper limit is
 * probably close to `atc_get_current_epoch_year() + 66`. To be conservative,
 * this function returns `atc_get_current_epoch_year() + 50`. It may return a
 * larger value in the future if the internal calculations can be verified to
 * avoid underflow or overflow problems.
 */
int16_t atc_epoch_valid_year_upper(void);

/**
 * Convert (year, month, day) triple to the number of days since the internal
 * epoch (2000-01-01). This algorithm corresponds to
 * AceTime/src/ace_time/internal/EpochConverterHinnant.h.
 *
 * No input validation is performed. The behavior is undefined if the
 * parameters are outside their expected range.
 *
 * @param year [1,9999]
 * @param month month integer, [1,12]
 * @param day day of month integer, [1,31]
 */
int32_t atc_convert_to_internal_days(int16_t year, uint8_t month, uint8_t day);

/**
 * Convert the days from internal epoch (2000-01-01) into (year, month, day)
 * fields. This algorithm corresponds to
 * AceTime/src/ace_time/internal/EpochConverterHinnant.h.
 *
 * No input validation is performed. The behavior is undefined if the
 * parameters are outside their expected range.
 *
 * @param internal_days number of days from internal epoch of 2000-01-01
 * @param year year [1,9999]
 * @param month month integer [1, 12]
 * @param day day of month integer[1, 31]
 */
void atc_convert_from_internal_days(
    int32_t internal_days,
    int16_t *year,
    uint8_t *month,
    uint8_t *day);

#ifdef __cplusplus
}
#endif

#endif
