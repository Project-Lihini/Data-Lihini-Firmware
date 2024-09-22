/*
 * Project Name: Project Lihini
 * File Name: wifi_conn.h
 * Author: Asanka Sovis
 * Created: 19/09/2024
 * Description: All functions related to WiFi connectivity, DNS resolution, SNTP
 * and Timezone handling.
 *
 * Modified By: Asanka Sovis
 * Modified: 22/09/2024
 * 
 * Changelog:
 *   - Initial Commit
 * 
 * Reference:
 *   - https://github.com/espressif/esp8266-rtos-sample-code/tree/master/03Wifi/Soft_AP_DEMO
 *   - https://docs.espressif.com/projects/esp-idf/en/v4.4.1/esp32/api-reference/system/system_time.html#system-time-sntp-sync
 *   - https://en.wikipedia.org/wiki/Unix_time
 *   - https://time.is/Unix_time_converter
 *
 * Copyright (C) 2024 Project Lihini. All rights reserved.
 */

#ifndef LWIP_TIMEVAL_PRIVATE
#define LWIP_TIMEVAL_PRIVATE 0
#endif

#include "esp_common.h"
#include "espconn.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "lwip/sockets.h"
#include "esp_system.h"
#include "lwip/netdb.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_wifi.h"
#include "sys/time.h"
#include "time.h"
#include "lwip/apps/sntp.h"

#include "../../include/app_conf.h"
#include "acetime/acetimec.h"

// Type for WiFi status is defined already in the esp_wifi.h CONNECTION_IN_PROGRESS
// is added to this in addition to define the state where it's still connecting.
#define CONNECTION_IN_PROGRESS 9

// Type to hold the connection status
typedef enum {
    CONNECTION_SUCCESS,                 // Success
    CONNECTION_DNS_RESOLUTION_FAIL,     // DNS fail
    CONNECTION_SOCKET_FAIL,             // Socket connection fail
    CONNECTION_FAIL,                    // Connection fail
    SNTP_ERROR,                         // SNTP error
    MAC_ADDRESS_NOT_SET                 // MAC address not yet updated
} CONNECTION_STATUS;

void connect_wifi(char *ssid, char *passwordsntp_sync_status_t);
int wifi_status_decode(int status);
void wifi_conn_event(System_Event_t *evt);
uint8 identifier_resolve();
int get_ip();
int ping();
int update_ntp_time(char *sntp_server);
int get_ntp_time(u_long *ntp_time);
u_long shift_timezone(u_long ntp_time);
u_long get_time();
void time_to_str(char *timestamp, u_long time);
