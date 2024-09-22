/*
 * MIT License
 * Copyright (c) 2022 Brian T. Park
 */

#include <stdint.h>
#include "common.h"
#include "epoch.h"

// Initialized to the default current epoch. If this is changed, then the
// atc_days_to_current_epoch_from_internal must be changed as well.
int16_t atc_current_epoch_year = kAtcDefaultEpochYear;

// Number of days from 2000 (internal epoch) to 2050 (default current epoch):
int32_t atc_days_to_current_epoch_from_internal_epoch =
    kAtcDaysToDefaultEpochFromInternalEpoch;

int16_t atc_get_current_epoch_year(void)
{
  return atc_current_epoch_year;
}

void atc_set_current_epoch_year(int16_t year)
{
  atc_current_epoch_year = year;
  atc_days_to_current_epoch_from_internal_epoch =
      atc_convert_to_internal_days(year, 1, 1);
}

int16_t atc_epoch_valid_year_lower(void)
{
  return atc_get_current_epoch_year() - 50;
}

int16_t atc_epoch_valid_year_upper(void)
{
  return atc_get_current_epoch_year() + 50;
}

int64_t atc_unix_seconds_from_epoch_seconds(atc_time_t epoch_seconds)
{
  if (epoch_seconds == kAtcInvalidEpochSeconds) {
    return kAtcInvalidUnixSeconds;
  } else {
    return (int64_t) epoch_seconds
        + (int64_t) 86400
          * (atc_days_to_current_epoch_from_internal_epoch
              + kAtcDaysToInternalEpochFromUnixEpoch);
  }
}

atc_time_t atc_epoch_seconds_from_unix_seconds(int64_t unix_seconds)
{
  if (unix_seconds == kAtcInvalidUnixSeconds) {
    return kAtcInvalidEpochSeconds;
  } else {
    return (int64_t) unix_seconds
        - (int64_t) 86400
          * (atc_days_to_current_epoch_from_internal_epoch
              + kAtcDaysToInternalEpochFromUnixEpoch);
  }
}

int32_t atc_unix_days_from_epoch_days(int32_t epoch_days)
{
  return epoch_days
      + atc_days_to_current_epoch_from_internal_epoch
      + kAtcDaysToInternalEpochFromUnixEpoch;
}

int32_t atc_epoch_days_from_unix_days(int32_t unix_days)
{
  return unix_days
      - kAtcDaysToInternalEpochFromUnixEpoch
      - atc_days_to_current_epoch_from_internal_epoch;
}

// Return the number days before the given month_prime.
// See AceTime/src/ace_time/internal/EpochConverterHinnant.h.
static uint16_t atc_convert_to_days_until_month_prime(uint8_t month_prime)
{
  return (153 * month_prime + 2) / 5;
}

// See AceTime/src/ace_time/internal/EpochConverterHinnant.h.
int32_t atc_convert_to_internal_days(int16_t year, uint8_t month, uint8_t day)
{
  int16_t year_prime = year - ((month <= 2) ? 1 : 0); // year begins on Mar 1
  uint16_t era = year_prime / 400; // [0,24]
  uint16_t year_of_era = year_prime - 400 * era; // [0,399]

  uint8_t month_prime = (month <= 2) ? month + 9 : month - 3; // [0,11]
  uint16_t days_until_month_prime =
      atc_convert_to_days_until_month_prime(month_prime);
  uint16_t day_of_year_prime = days_until_month_prime + day - 1; // [0,365]
  uint32_t day_of_era = (uint32_t) 365 * year_of_era + (year_of_era / 4)
      - (year_of_era / 100) + day_of_year_prime; // [0,146096]

  // epoch_prime days is relative to 0000-03-01
  int32_t day_of_epoch_prime = day_of_era + 146097 * era;
  return day_of_epoch_prime
          - (kAtcInternalEpochYear / 400) * 146097 // relative to 2000-03-01
          + 60; // relative to 2000-01-01, 2000 is a leap year
}

// See AceTime/src/ace_time/internal/EpochConverterHinnant.h.
void atc_convert_from_internal_days(
    int32_t internal_days,
    int16_t *year,
    uint8_t *month,
    uint8_t *day)
{
  // epoch_prime days is relative to 0000-03-01
  int32_t day_of_epoch_prime = internal_days
      + (kAtcInternalEpochYear / 400) * 146097
      - 60;

  uint16_t era = (uint32_t) day_of_epoch_prime / 146097; // [0,24]
  uint32_t day_of_era = day_of_epoch_prime - 146097 * era; // [0,146096]
  uint16_t year_of_era = (day_of_era - day_of_era / 1460 + day_of_era / 36524
      - day_of_era / 146096) / 365; // [0,399]
  uint16_t year_prime = year_of_era + 400 * era; // [0,9999]
  uint16_t day_of_year_prime = day_of_era
      - (365 * year_of_era + year_of_era / 4 - year_of_era/100); // [0,365]
  uint8_t month_prime = (5 * day_of_year_prime + 2) / 153;
  uint16_t days_until_month_prime =
      atc_convert_to_days_until_month_prime(month_prime);

  *day = day_of_year_prime - days_until_month_prime + 1; // [1,31]
  *month = (month_prime < 10) ? month_prime + 3 : month_prime - 9; // [1,12]
  *year = year_prime + ((*month <= 2) ? 1 : 0); // [1,9999]
}
