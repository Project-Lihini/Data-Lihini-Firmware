// This file was generated by the following script:
//
//   $ /home/brian/src/AceTimeTools/src/acetimetools/tzcompiler.py
//     --input_dir /home/brian/src/acetimec/src/zonedbtesting/tzfiles
//     --output_dir /home/brian/src/acetimec/src/zonedbtesting
//     --tz_version 2024a
//     --actions zonedb
//     --languages c
//     --scope complete
//     --db_namespace AtcTesting
//     --start_year 2000
//     --until_year 2200
//     --nocompress
//     --include_list include_list.txt
//
// using the TZ Database files
//
//   africa
//   antarctica
//   asia
//   australasia
//   backward
//   etcetera
//   europe
//   northamerica
//   southamerica
//
// from https://github.com/eggert/tz/releases/tag/2024a
//
// Supported Zones: 17 (16 zones, 1 links)
// Unsupported Zones: 579 (335 zones, 244 links)
//
// Requested Years: [2000,2200]
// Accurate Years: [2000,32767]
//
// Original Years:  [1844,2087]
// Generated Years: [1966,2087]
// Lower/Upper Truncated: [True,False]
//
// Estimator Years: [1966,2090]
// Max Buffer Size: 7
//
// Records:
//   Infos: 17
//   Eras: 22
//   Policies: 8
//   Rules: 201
//
// Memory (8-bits):
//   Context: 16
//   Rules: 2412
//   Policies: 24
//   Eras: 330
//   Zones: 208
//   Links: 13
//   Registry: 34
//   Formats: 78
//   Letters: 23
//   Fragments: 0
//   Names: 268 (original: 268)
//   TOTAL: 3406
//
// Memory (32-bits):
//   Context: 24
//   Rules: 2412
//   Policies: 64
//   Eras: 440
//   Zones: 384
//   Links: 24
//   Registry: 68
//   Formats: 78
//   Letters: 33
//   Fragments: 0
//   Names: 268 (original: 268)
//   TOTAL: 3795
//
// DO NOT EDIT

#include "zone_infos.h"
#include "zone_registry.h"

//---------------------------------------------------------------------------
// Zone Info registry. Sorted by zoneId.
//---------------------------------------------------------------------------
const AtcZoneInfo * const kAtcTestingZoneRegistry[16]  = {
  &kAtcTestingZoneAmerica_New_York, // 0x1e2a7654, America/New_York
  &kAtcTestingZonePacific_Apia, // 0x23359b5e, Pacific/Apia
  &kAtcTestingZoneAustralia_Darwin, // 0x2876bdff, Australia/Darwin
  &kAtcTestingZoneAmerica_Vancouver, // 0x2c6f6b1f, America/Vancouver
  &kAtcTestingZoneAmerica_Caracas, // 0x3be064f4, America/Caracas
  &kAtcTestingZoneAmerica_Chicago, // 0x4b92b5d4, America/Chicago
  &kAtcTestingZoneAmerica_Whitehorse, // 0x54e0e3e8, America/Whitehorse
  &kAtcTestingZoneEurope_Lisbon, // 0x5c00a70b, Europe/Lisbon
  &kAtcTestingZoneAmerica_Edmonton, // 0x6cb9484a, America/Edmonton
  &kAtcTestingZoneAfrica_Windhoek, // 0x789c9bd3, Africa/Windhoek
  &kAtcTestingZoneAmerica_Toronto, // 0x792e851b, America/Toronto
  &kAtcTestingZoneAmerica_Winnipeg, // 0x8c7dafc7, America/Winnipeg
  &kAtcTestingZoneAmerica_Denver, // 0x97d10b2a, America/Denver
  &kAtcTestingZoneAmerica_Los_Angeles, // 0xb7f7e8f2, America/Los_Angeles
  &kAtcTestingZoneAfrica_Casablanca, // 0xc59f1b33, Africa/Casablanca
  &kAtcTestingZoneEtc_UTC, // 0xd8e31abc, Etc/UTC

};

//---------------------------------------------------------------------------
// Zone and Link Info registry. Sorted by zoneId. Links act like Zones.
//---------------------------------------------------------------------------
const AtcZoneInfo * const kAtcTestingZoneAndLinkRegistry[17]  = {
  &kAtcTestingZoneAmerica_New_York, // 0x1e2a7654, America/New_York
  &kAtcTestingZonePacific_Apia, // 0x23359b5e, Pacific/Apia
  &kAtcTestingZoneAustralia_Darwin, // 0x2876bdff, Australia/Darwin
  &kAtcTestingZoneAmerica_Vancouver, // 0x2c6f6b1f, America/Vancouver
  &kAtcTestingZoneAmerica_Caracas, // 0x3be064f4, America/Caracas
  &kAtcTestingZoneAmerica_Chicago, // 0x4b92b5d4, America/Chicago
  &kAtcTestingZoneAmerica_Whitehorse, // 0x54e0e3e8, America/Whitehorse
  &kAtcTestingZoneEurope_Lisbon, // 0x5c00a70b, Europe/Lisbon
  &kAtcTestingZoneAmerica_Edmonton, // 0x6cb9484a, America/Edmonton
  &kAtcTestingZoneAfrica_Windhoek, // 0x789c9bd3, Africa/Windhoek
  &kAtcTestingZoneAmerica_Toronto, // 0x792e851b, America/Toronto
  &kAtcTestingZoneAmerica_Winnipeg, // 0x8c7dafc7, America/Winnipeg
  &kAtcTestingZoneAmerica_Denver, // 0x97d10b2a, America/Denver
  &kAtcTestingZoneUS_Pacific, // 0xa950f6ab, US/Pacific -> America/Los_Angeles
  &kAtcTestingZoneAmerica_Los_Angeles, // 0xb7f7e8f2, America/Los_Angeles
  &kAtcTestingZoneAfrica_Casablanca, // 0xc59f1b33, Africa/Casablanca
  &kAtcTestingZoneEtc_UTC, // 0xd8e31abc, Etc/UTC

};
