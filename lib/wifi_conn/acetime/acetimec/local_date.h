/*
 * MIT License
 * Copyright (c) 2022 Brian T. Park
 */

/**
 * @file local_date.h
 *
 * Low-level date functions, for example, for calculating leap years, day of
 * week, number of days in a specific month, and converting epoch seconds to
 * date-time components.
 *
 * Uses the algorithm described in
 * https://howardhinnant.github.io/date_algorithms.html.
 */

#ifndef ACE_TIME_C_LOCAL_DATE_H
#define ACE_TIME_C_LOCAL_DATE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** ISO Weekday numbers. Monday=1, Sunday=7. */
enum {
  kAtcIsoWeekdayMonday = 1,
  kAtcIsoWeekdayTuesday,
  kAtcIsoWeekdayWednesday,
  kAtcIsoWeekdayThursday,
  kAtcIsoWeekdayFriday,
  kAtcIsoWeekdaySaturday,
  kAtcIsoWeekdaySunday,
};

/** Return true if year is a leap year. */
bool atc_is_leap_year(int16_t year);

/** Return the number of days in the month for the given year. */
uint8_t atc_local_date_days_in_year_month(int16_t year, uint8_t month);

/**
 * Calculate the ISO day of week (i.e. Monday=1, Sunday=7) given the (year,
 * month, day). Idea borrowed from https://github.com/evq/utz. No validation of
 * year, month or day is performed.
 */
uint8_t atc_local_date_day_of_week(int16_t year, uint8_t month, uint8_t day);

/**
 * Return the number of days from the current epoch year to the (year, month,
 * day) triple.
 *
 * @param year [1,9999]
 * @param month month integer, [1,12]
 * @param day day of month integer, [1,31]
 */
int32_t atc_local_date_to_epoch_days(int16_t year, uint8_t month, uint8_t day);

/**
 * Extract the (year, month, day) fields from the current epoch year
 * defined by atc_set_current_epoch_year().
 *
 * No input validation is performed. The behavior is undefined if the
 * parameters are outside their expected range.
 *
 * @param epoch_days number of days from the current epoch year
 * @param year year [1,9999]
 * @param month month integer [1, 12]
 * @param day day of month integer[1, 31]
 */
void atc_local_date_from_epoch_days(
    int32_t epoch_days,
    int16_t *year,
    uint8_t *month,
    uint8_t *day);

/**
 * Return the number of days from the Unix epoch (1970) to the (year, month,
 * day) triple.
 *
 * @param year [1,9999]
 * @param month month integer, [1,12]
 * @param day day of month integer, [1,31]
 */
int32_t atc_local_date_to_unix_days(int16_t year, uint8_t month, uint8_t day);

/**
 * Extract the (year, month, day) fields from the number of days since Unix
 * epoch (1970).
 *
 * No input validation is performed. The behavior is undefined if the
 * parameters are outside their expected range.
 *
 * @param unix_days number of days from the Unix epoch
 * @param year year [1,9999]
 * @param month month integer [1, 12]
 * @param day day of month integer[1, 31]
 */
void atc_local_date_from_unix_days(
    int32_t unix_days,
    int16_t *year,
    uint8_t *month,
    uint8_t *day);

/** Increment given (year, month, day) by one day. */
void atc_local_date_increment_one_day(
    int16_t *year, uint8_t *month, uint8_t *day);

/** Decrement given (year, month, day) by one day. */
void atc_local_date_decrement_one_day(
    int16_t *year, uint8_t *month, uint8_t *day);

#ifdef __cplusplus
}
#endif

#endif
