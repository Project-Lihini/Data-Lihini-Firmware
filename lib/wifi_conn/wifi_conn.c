/*
 * Project Name: Project Lihini
 * File Name: wifi_conn.c
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

#include "wifi_conn.h"

// Constants ----------------------------------------------------------

#define SNTP_RECHECK_DELAY 100 // Delay after which to recheck the time var.
// NOTE: Time is assumed to be successfully updated if the received Unix time is
// above 905536800 (12/09/1998 00:00:00)

#define SNTP_EPOCH_THRESHOLD 905536800 // Value after which NTP update is assumed to
// be successful

// --------------------------------------------------------------------

// External Variables -------------------------------------------------

extern int8 wifi_status;            // WiFi conn.

extern char *target_host;           // Address
extern struct hostent *host;        // DNS resolution

extern char mac_address[18];        // MAC address of the ESP32
extern char unique_identifier[22];  //Unique identifier of the ESP32
// NOTE: This is used as the client ID for MQTT

// Timeshift to store the timezone deviation between ESPs NTP updated time (Relative
// to Asia/Shanghai) to the timezone used by the server for reference (Relative t0
// Asia/Colombo)
extern long timeshift;

// --------------------------------------------------------------------

/**
 * Connects to WiFi using the provided SSID and password. This only handles the
 * initial configuration. Connection is handled in the backend by the RTOS. State
 * is returned by the wifi_conn_event() function.
 * @param char *ssid WiFi SSID
 * @param char *password WiFi password
 * @return none
 */
void connect_wifi(char *ssid, char *password) {
    printf("Connecting: SSID %s | PW %s...\n", ssid, password);

	wifi_set_opmode(STATIONAP_MODE);
	struct station_config config;
	memset(&config,0,sizeof(config));  // Set value of config from address of &config
    // to width of size to be value '0'
    
	sprintf(config.ssid, ssid);
	sprintf(config.password, password);
    
	wifi_station_set_config(&config);         
	wifi_set_event_handler_cb(wifi_conn_event);
	wifi_station_connect();

    wifi_status = CONNECTION_IN_PROGRESS;
}

/**
 * The connection sometimes go into EVENT_SOFTAPMODE_PROBEREQRECVED which is also
 * a sustained connection. This function returns true if either
 * EVENT_SOFTAPMODE_PROBEREQRECVED or EVENT_STAMODE_GOT_IP for determining if
 * WiFi connection is ok.
 * @param int status WiFi status
 * @return none
 */
int wifi_status_decode(int status) {
    return (
        (status == EVENT_STAMODE_GOT_IP) ||
        (status == EVENT_SOFTAPMODE_PROBEREQRECVED)
    );
}

/**
 * This function is called by the OS when a WiFi event is triggered. This function
 * checks these statuses and sets the wifi_status flag to this event. The status
 * is also printed.
 * @param System_Event_t *evt WiFi conn. event
 * @return none
 */
void wifi_conn_event(System_Event_t *evt) {
    // Setting the status to wifi_status flag
    wifi_status = evt->event_id;

    printf("Station: " MACSTR "join, AID: %d\n", MAC2STR(evt->event_info.sta_connected.mac),
                    evt->event_info.sta_connected.aid);

    switch (evt->event_id) {
        case EVENT_STAMODE_CONNECTED:
            printf("Connected to: SSID %s | Channel %d\n", evt->event_info.connected.ssid,
                    evt->event_info.connected.channel);
            break;
        case EVENT_STAMODE_DISCONNECTED:
            printf("Disconnect from: SSID %s | Reason %d\n", evt->event_info.disconnected.ssid,
                    evt->event_info.disconnected.reason);
            break;
        case EVENT_STAMODE_AUTHMODE_CHANGE:
            printf("Mode switch: %d -> %d\n", evt->event_info.auth_change.old_mode, evt->event_info.auth_change.new_mode);
            break;
        case EVENT_STAMODE_GOT_IP:
            printf("IP:" IPSTR " | Mask:" IPSTR " | GW:" IPSTR, IP2STR(&evt->event_info.got_ip.ip),
                    IP2STR(&evt->event_info.got_ip.mask), IP2STR(&evt->event_info.got_ip.gw));
            printf("\n");
            sprintf(mac_address, MACSTR, MAC2STR(evt->event_info.sta_disconnected.mac)); // Set MAC addr.
            identifier_resolve(); // Resolve identifier
            break;
        case EVENT_SOFTAPMODE_STACONNECTED:
            printf("Connected | Station: " MACSTR " | AID: %d\n", MAC2STR(evt->event_info.sta_connected.mac),
                    evt->event_info.sta_connected.aid);
            break;
        case EVENT_SOFTAPMODE_STADISCONNECTED:
            printf("Discted | Station: " MACSTR " | AID: %d\n", MAC2STR(evt->event_info.sta_disconnected.mac),
                    evt->event_info.sta_disconnected.aid);
            break;
        default:
            break;
    }
}

/**
 * This function generates a unique identifier to identify the specific device.
 * This identifier is generated using the MAC address of the device when connected
 * to a network. This is typically in the form XX:XX:XX:XX:XX where X can be from
 * 0 to f in hex. ex: c0:a8:01:02:ff:ff. The unique identifier is of the form
 * lihini_XX:XX:XX:XX:XX. ex: lihini_c0:a8:01:02:ff:ff. This unique identifier is
 * used for MQTT client ID.
 *      MAC_ADDRESS_NOT_SET             - MAC address not yet updated
 *      CONNECTION_SUCCESS              - ID creation success
 * @param none
 * @return int Success/Fail
 */
uint8 identifier_resolve() {
    // If a MAC address is available and unique identifier is not yet set, a
    // unique identifier is generated.
    if (strncmp(mac_address, "\0", 1) && (!strncmp(unique_identifier, "\0", 1))) {
        printf("MAC address resolve: %s\n", mac_address);
        sprintf(unique_identifier, "lihini_%s", mac_address);
        printf("Unique identifier set: %s\n", unique_identifier);

        return CONNECTION_SUCCESS;
    }

    return MAC_ADDRESS_NOT_SET;
}

/**
 * Returns the IP address from a given hostname by connecting to a DNS server.
 *      CONNECTION_DNS_RESOLUTION_FAIL  - DNS fail
 *      CONNECTION_SUCCESS              - DNS success
 * @param none
 * @return int Success/Fail
 */
int get_ip() {
    host = gethostbyname(target_host);

    if (host == NULL) {
        printf("DNS resolution failed.\n");
        return CONNECTION_DNS_RESOLUTION_FAIL;
    } else {
        printf("DNS resolution successful.\n");
        return CONNECTION_SUCCESS;
    }
}

/**
 * Pings the provided IP address of the host and returns the success.
 *      CONNECTION_SOCKET_FAIL          - Socket connection fail
 *      CONNECTION_FAIL                 - Connection fail
 *      CONNECTION_SUCCESS              - Success
 * @param none
 * @return int Success/Fail
 */
int ping() {
    // Socket creation
    struct sockaddr_in addr;
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock == -1) {
        printf("Failed to create socket.\n");
        return CONNECTION_SOCKET_FAIL;
    }

    printf("Socket created.\n");

    // Connection

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);

    char *ip_addr = inet_ntoa(*host->h_addr_list[0]);

    printf("Connecting: %s...\n", ip_addr);
    inet_aton(ip_addr, &addr.sin_addr);

    int ret = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
    if (ret == -1) {
        printf("Connection failure.\n");
        close(sock);
        return CONNECTION_FAIL;
    }

    printf("Connection successful.\n");

    close(sock); // Socket close

    return CONNECTION_SUCCESS;
}

/**
 * This function is called by the threads when NTP time needs to be
 * first updated. This initializes the NTP time related functions of
 * the OS and call get_ntp_time() to update it.
 *      SNTP_ERROR                      - SNTP error
 *      CONNECTION_SUCCESS              - Success
 * 
 * NOTE: ESP8266 returns the time formatted in the Unix timestamp format.
 * Unix timestamp is a 32 bit integer with time passed as of 00:00:00 on
 * 01/01/1970. The ESP8266 returns the time referenced to the timezone
 * Asia/Shanghai.
 * Reference: https://docs.espressif.com/projects/esp-idf/en/v4.4.1/esp32/api-reference/system/system_time.html#system-time-sntp-sync
 * Reference: https://en.wikipedia.org/wiki/Unix_time
 * Timestamp conversion: https://time.is/Unix_time_converter
 * 
 * @param char *sntp_server NTP server URL
 * @return int Success/Fail
 */
int update_ntp_time(char *sntp_server) {
    printf("Updating the SNTP time: Server: %s...\n", sntp_server);

    // SNTP init
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, sntp_server);
    sntp_init();

    printf("Waiting for SNTP update...\n");

    // Get NTP time
    u_long ntp_time = 0;
    int error = get_ntp_time(&ntp_time);

    if (error == SNTP_ERROR) {
        printf("SNTP update timed out. Will retry again...\n");
    } else {
        printf("NTP time updated. Time: %d\n", ntp_time);
    }

    return error;
}

/**
 * Gets the NTP time from the provided NTP server. NTP server is provided in
 * DEFAULT_SNTP_SERVER. The received NTP time is stored in *ntp_time.
 *      SNTP_ERROR                      - SNTP error
 *      CONNECTION_SUCCESS              - Success
 * @param u_long *ntp_time Pointer to store the time
 * @return int Success/Fail
 */
int get_ntp_time(u_long *ntp_time) {
    struct timeval tv = {0}; // Received SNTP time

    int count = 0; // Timeout counter

    // Wait while NTP time is updated
    while (tv.tv_sec < SNTP_EPOCH_THRESHOLD) {
        vTaskDelay(SNTP_RECHECK_DELAY);
        gettimeofday(&tv, NULL);

        if (count > SNTP_UPDATE_TIMEOUT) {
            return SNTP_ERROR;
        }

        count += SNTP_RECHECK_DELAY;
    }

    // Time shifted to Asia/Colombo
    u_long current_time = shift_timezone(tv.tv_sec);

    *ntp_time = current_time;
    timeshift =  current_time - (u_long)tv.tv_sec;

    return CONNECTION_SUCCESS;
}

/**
 * Calculates the amount of seconds to be shifted using AceTime C library.
 * The process is resource-heavy, thus the shift in seconds is calculated
 * and stored for later. [Asia/Shanghai -> Asia/Colombo]
 * @param u_long ntp_time NTP time received
 * @return u_long Corrected NTP time
 */
u_long shift_timezone(u_long ntp_time) {
    // Convert the NTP time received to the local data format
    AtcLocalDateTime current_time = {0};

    atc_local_date_time_from_unix_seconds(&current_time, ntp_time);

    // Attaching the Shanghai timezone because ESP8266 request NTP time
    // for timezone Asia/Shanghai

    AtcZoneProcessor processor_sh;
    atc_processor_init(&processor_sh);
    AtcTimeZone tz_sh = {&kAtcZoneAsia_Shanghai, &processor_sh};

    AtcZonedDateTime current_time_sh = {0};

    atc_zoned_date_time_from_local_date_time(&current_time_sh, &current_time, &tz_sh);

    // Converting Asia/Shanghai time to Asia/Colombo time

    AtcZoneProcessor processor_sl;
    atc_processor_init(&processor_sl);
    AtcTimeZone tz_sl = {&DEFAULT_TIMEZONE, &processor_sl};

    AtcZonedDateTime current_time_lk = {0};

    atc_zoned_date_time_convert(&current_time_sh, &tz_sl, &current_time_lk);

    // Converting Asia/Colombo time to local NTP time

    AtcLocalDateTime current_time_loc = {
        .day = current_time_lk.day,
        .month = current_time_lk.month,
        .year = current_time_lk.year,
        .hour = current_time_lk.hour,
        .minute = current_time_lk.minute,
        .second = current_time_lk.second
    };

    ntp_time = atc_local_date_time_to_unix_seconds(&current_time_loc);
    
    printf("%d.%d.%d %d:%d:%d %s\n", 
        current_time_lk.day, 
        current_time_lk.month, 
        current_time_lk.year, 
        current_time_lk.hour, 
        current_time_lk.minute, 
        current_time_lk.second, 
        current_time_lk.tz.zone_info->name
    );

    return ntp_time;
}

/**
 * Returns the current time using gettimeofday(). Only to be used after
 * update_ntp_time() is called. It can only return correct time once
 * NTP time is correctly updated.
 * @param none
 * @return u_long Current time
 */
u_long get_time() {
    struct timeval tv = {0};

    gettimeofday(&tv, NULL);

    return (tv.tv_sec + timeshift);
}

/**
 * Creates a string with the date and time.
 * DD-MM-YYYY hh:mm:ss
 * @param char *timestamp Generated timestamp
 * @param u_long time Current time in Unix secs.
 * @return none
 */
void time_to_str(char *timestamp, u_long time) {
    AtcLocalDateTime current_time = {0};

    // Unix time -> Local time (Hours, Mins, etc. separate)
    atc_local_date_time_from_unix_seconds(&current_time, time);

    // Formatting to add a 0 at the front if number is < 10
    int values[5] = {current_time.day, current_time.month, current_time.hour, current_time.minute, current_time.second};
    char str_values[5][3] = {0};

    for (int i = 0; i < 5; i++) {
        if (values[i] < 10)
            sprintf(str_values[i], "0%d\0", values[i]);
        else
            sprintf(str_values[i], "%d\0", values[i]);
    }
    
    sprintf(timestamp, "%s-%s-%d %s:%s:%s\0", str_values[0], str_values[1], current_time.year, str_values[2], str_values[3], str_values[4]);
}
