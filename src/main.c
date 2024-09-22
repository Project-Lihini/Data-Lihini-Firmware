/*
 * ESPRSSIF MIT License
 *
 * Copyright (c) 2015 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/*
 * Project Name: Project Lihini
 * File Name: main.c
 * Author: Asanka Sovis
 * Created: 10/09/2024
 * Description: The main() entry point of program.
 *
 * Modified By: Asanka Sovis
 * Modified: 22/09/2024
 * 
 * Changelog:
 *   - Initial Commit
 *
 * Copyright (C) 2024 Project Lihini. All rights reserved.
 */

#ifndef LWIP_TIMEVAL_PRIVATE
#define LWIP_TIMEVAL_PRIVATE 0
#endif

#include "esp_common.h"
#include "lwip/netdb.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "freertos/timers.h"
#include "sys/time.h"
#include "freertos/queue.h"
#include "gpio.h"

#include "app_conf.h"
#include "../lib/wifi_conn/wifi_conn.h"
#include "../lib/mqtt_conn/mqtt_conn.h"
#include "../lib/wifi_conn/acetime/acetimec.h"

// Constants ----------------------------------------------------------

#define ERROR_INDICATOR_DELAY 10 // Freq of error indicator of publish
// fail
#define LIVE_INDICATOR_DELAY 100 // Live indicator thread start delay
#define SLOW_PULSE_FREQ 100 // LED slow pulse freq
#define QUICK_PULSE_FREQ 10 // LED quick pulse freq

#define NETWORK_MANAGEMENT_DELAY 100 // Network management delay
#define MQTT_PROCESS_DELAY 100// MQTT process delay
#define THREAD_MONITOR_DELAY 1000 // Thread monitor delay

// --------------------------------------------------------------------

// Status Flags--------------------------------------------------------
// These are set to indicate the status of each task.

int8 wifi_status = EVENT_STAMODE_DISCONNECTED;  // WiFi conn.
int8 connection_status = CONNECTION_FAIL;       // Internet
int8 ntp_status = SNTP_ERROR;                   // SNTP status
int8 mqtt_status = MQTT_DISCONNECT;             // MQTT status

// --------------------------------------------------------------------

// Thread Handles -----------------------------------------------------
// These hold the handles to each main thread of Free RTOS.

xTaskHandle network_monitor_handle = NULL;      // Network
xTaskHandle mqtt_monitor_handle = NULL;         // MQTT
xTaskHandle live_indication_handle = NULL;      // Indicators

// --------------------------------------------------------------------

// Watchdog Variables -------------------------------------------------
// These are reset at variable intervals by each thread. The task
// monitor will set these at fixed intervals. If the thread fails to
// reset, the thread is stopped and restarted.

uint8 network_monitor_reset = 0;                // Network
uint8 mqtt_monitor_reset = 0;                   // MQTT
uint8 live_indication_reset = 0;                // Indicators

// --------------------------------------------------------------------

char *wifi_ssid = DEFAULT_WIFI_SSID;            // WiFi SSID
char *wifi_password = DEFAULT_WIFI_PASSWORD;    // WiFi Password
char *sntp_server = DEFAULT_SNTP_SERVER;        // SNTP Server

// Host to check the connectivity of the network
char *target_host = DEFAULT_CONN_SERVER;        // Address
struct hostent *host = {0};                     // DNS resolution

// Current subscribe topic
char *current_subscribe_topic = MQTT_SUBSCRIBE_TOPIC;

// c0:a8:01:02:ff:ff
char mac_address[18] = {0};                     // MAC address of the ESP32
char unique_identifier[22] = {0};               //Unique identifier of the ESP32
// NOTE: This is used as the client ID for MQTT [lihini_c0:a8:01:02:ff:ff]

// Main queues
xQueueHandle incoming_queue = {0};              // Incoming from server
xQueueHandle outgoing_queue = {0};              // Outgoing to server

// Timeshift to store the timezone deviation between ESPs NTP updated time (Relative
// to Asia/Shanghai) to the timezone used by the server for reference (Relative t0
// Asia/Colombo)
long timeshift = 0;

void task_monitor();
void network_monitor();
void mqtt_monitor();
void live_indication();
void fake_mqtt_traffic();
void timed_interrupt_callback(xTimerHandle interrupt_timer);
void create_timed_interrupt();

/**
 * Calculates the factorial of a given non-negative integer.
 * SDK just reversed 4 sectors, used for rf init data and paramters. We add this function
 * to force users to set rf cal sector, since we don't know which sector is free in user
 * #include "time.h"'s application. sector map for last several sectors : ABCCC
 * A : rf cal
 * B : rf init data
 * C : sdk parameters
 * @param none
 * @return rf cal sector
 */
uint32 user_rf_cal_sector_set(void) {
    flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

/**
 * Entry of user application, init user function here
 * @param none
 * @return none
 */
void user_init(void)
{
    //create_timed_interrupt();

    xTaskCreate(task_monitor, "task_monitor", 500, NULL, 6, NULL); // Tasnk monitor
    xTaskCreate(fake_mqtt_traffic, "fake_mqtt_traffic", 500, NULL, 6, NULL); // MQTT Traffic
}

/**
 * Primary thread that manages the service threads. All threads have a corresponding watchdog
 * variable. The task monitor sets this at fixed interval and the corresponding thread is
 * expected to reset it at variable intervals. If the thread fails to reset the variable,
 * task monitor assumes the thread to be hanged and restarts it.
 * @param none
 * @return none
 */
void task_monitor() {
    while (TRUE) {
        // Networking thread reset
        if (network_monitor_reset) {
            printf("Network thread has timed out. Restarting thread...\n");
            if (network_monitor_handle != NULL) {
                vTaskDelete(network_monitor_handle);
                network_monitor_handle = NULL;
            }
        }

        // MQTT thread reset
        if (mqtt_monitor_reset) {
            printf("MQTT thread has timed out. Restarting thread...\n");
            if (mqtt_monitor_handle != NULL) {
                vTaskDelete(mqtt_monitor_handle);
                mqtt_monitor_handle = NULL;
            }
        }

        // Indicator thread reset
        if (live_indication_reset) {
            printf("Indication thread has timed out. Restarting thread...\n");
            if (live_indication_handle != NULL) {
                vTaskDelete(live_indication_handle);
                live_indication_handle = NULL;
            }
        }

        // Network thread create
        if (network_monitor_handle == NULL) {
            xTaskCreate(network_monitor, "network_monitor", 1600, NULL, 6, &network_monitor_handle);
        }

        // MQTT thread create
        if (mqtt_monitor_handle == NULL) {
            xTaskCreate(mqtt_monitor, "mqtt_monitor", 1000, NULL, 6, &mqtt_monitor_handle);
        }

        // Indicator thread create
        if (live_indication_handle == NULL) {
            xTaskCreate(live_indication, "live_indication", 500, NULL, 6, &live_indication_handle);
        }

        // Setting watchdog variables
        network_monitor_reset = 1;
        mqtt_monitor_reset = 1;
        live_indication_reset = 1;

        vTaskDelay(THREAD_MONITOR_DELAY);
    }

    vTaskDelete(NULL);
}

/**
 * Thread that monitors the network state of the device. These include WiFi connection,
 * internet connection and SNTP update. Each will depend on the success of the others.
 * State of each is defined in the wifi_conn.h.
 * @param none
 * @return none
 */
void network_monitor() {
    printf("Network monitor starting...\n\0");

    while (TRUE) {
        // Check if WiFi status went into STATION_GOT_IP which indicate successful
        // connection to the network.
        if (wifi_station_get_connect_status() == STATION_GOT_IP) {
            wifi_status = EVENT_STAMODE_GOT_IP;
        }

        // If WiFi status is disconnected, connect
        if (!wifi_status_decode(wifi_status)) {
            connection_status = CONNECTION_FAIL;
            ntp_status = SNTP_ERROR;
            mqtt_status = MQTT_DISCONNECT;

            connect_wifi(wifi_ssid, wifi_password);

            vTaskDelay(NETWORK_MANAGEMENT_DELAY);
        }

        // If WiFi status is connected but internet is disconnected, wait until
        // connectivity is available
        if (wifi_status_decode(wifi_status) && (connection_status != CONNECTION_SUCCESS)) {
            ntp_status = SNTP_ERROR;
            mqtt_status = MQTT_DISCONNECT;

            connection_status = get_ip();

            if (connection_status == CONNECTION_SUCCESS)
                connection_status = ping();

            vTaskDelay(NETWORK_MANAGEMENT_DELAY);
        }

        // If WiFi status is connected and internet ping is successful, update NTP
        if (wifi_status_decode(wifi_status) && (connection_status == CONNECTION_SUCCESS) && (ntp_status == SNTP_ERROR)) {
            mqtt_status = MQTT_DISCONNECT;
            ntp_status = update_ntp_time(sntp_server);

            vTaskDelay(NETWORK_MANAGEMENT_DELAY);
        }

        //printf("wifi_status:%d | connection_status: %d | ntp_status: %d", wifi_status, connection_status, ntp_status);
        network_monitor_reset = 0; // Watchdog reset

        vTaskDelay(NETWORK_MANAGEMENT_DELAY);
    }

    vTaskDelete(NULL);
}

/**
 * Thread that monitors the MQTT status and take actions related to MQTT. These include
 * handling and publishing MQTT queues. Receiving MQTT from server if available and
 * enqueuing this data.
 * @param none
 * @return none
 */
void mqtt_monitor() {
    printf("MQTT monitor starting...\n\0");

    mqtt_queue_init(MAX_QUEUE_SIZE); // Initialize MQTT queues

    while (TRUE) {
        // MQTT thread handles MQTT only if WiFi is connected, internet is working and
        // NTP time is updated
        if (wifi_status_decode(wifi_status) && (connection_status == CONNECTION_SUCCESS) && (ntp_status == CONNECTION_SUCCESS)) {
            // Check for the availability of a unique identifier
            if (strncmp(unique_identifier, "\0", 1)) {
                // Connect MQTT
                mqtt_status = mqtt_connect(DEFAULT_MQTT_SERVER, unique_identifier, MQTT_PORT, MQTT_TIMEOUT, MQTT_BUFF_SIZE);

                // If MQTT connection was successful
                if (mqtt_status == MQTT_CONNECTION_SUCCESS) {
                    mqtt_status = MQTT_ACTIVE;

                    // Publish MQTT queue
                    uint8 error = mqtt_queue_publish();

                    // Queue publish was successful
                    if (error == MQTT_QUEUE_SUCCESS) {
                        mqtt_status = MQTT_CONNECTION_SUCCESS;

                        // Subscribe MQTT topic
                        error = mqtt_check_topic(current_subscribe_topic, QOS1);

                        // Subscribe status
                        if (error == MQTT_CONNECTION_DISCONNECT) {
                            mqtt_status = MQTT_DISCONNECT;
                        } else {
                            printf("~~~\n");
                            mqtt_status = MQTT_CONNECTION_SUCCESS;
                        }
                    } else {
                        // Publish fail due to MQTT disconnect. Request network
                        // thread to reconnect
                        if (error == MQTT_CONNECTION_DISCONNECT) {
                            mqtt_status = MQTT_DISCONNECT;
                        } else {
                            mqtt_status = MQTT_PUBLISH_FAIL;
                        }

                        // Indicate publish failure
                        GPIO_OUTPUT_SET(INDICATION_LED_1, 1);
                        vTaskDelay(ERROR_INDICATOR_DELAY);
                        GPIO_OUTPUT_SET(INDICATION_LED_1, 0);
                        vTaskDelay(ERROR_INDICATOR_DELAY);
                        GPIO_OUTPUT_SET(INDICATION_LED_1, 1);
                        vTaskDelay(ERROR_INDICATOR_DELAY);
                        GPIO_OUTPUT_SET(INDICATION_LED_1, 0);
                        vTaskDelay(ERROR_INDICATOR_DELAY);
                    }
                }

                mqtt_disconnect(); // Disconnect MQTT
            } else {
                // Resolving the unique identifier unavailability
                if (identifier_resolve() == MAC_ADDRESS_NOT_SET) {
                    printf("No unique identifier is yet processed.");
                }
            }
        }

        mqtt_monitor_reset = 0; // Watchdog reset

        vTaskDelay(MQTT_PROCESS_DELAY);
    }

    vTaskDelete(NULL);
}

/**
 * This thread handles indication for LEDs in the device. Indication includes two
 * LEDs: Red LED, Green LED. Red LED indicates progress and error states. Green
 * indicate publishing status.
 *      Slow pulsing Red        - Connecting to WiFi
 *      Fast pulsing Red        - Checking network and NTP update
 *      Two fast Red LED pulses - MQTT publish fail
 *      Fast pulsing Green LED  - MQTT publishing
 * @param none
 * @return none
 */
void live_indication() {
    // Red LED config.
    GPIO_ConfigTypeDef led_1;
    led_1.GPIO_IntrType = GPIO_PIN_INTR_DISABLE;
    led_1.GPIO_Mode = GPIO_Mode_Output;
    led_1.GPIO_Pin = GPIO_Pin_14;
    led_1.GPIO_Pullup = GPIO_PullUp_EN;
    gpio_config(&led_1);

    // Green LED config.
    GPIO_ConfigTypeDef led_2;
    led_2.GPIO_IntrType = GPIO_PIN_INTR_DISABLE;
    led_2.GPIO_Mode = GPIO_Mode_Output;
    led_2.GPIO_Pin = GPIO_Pin_12;
    led_2.GPIO_Pullup = GPIO_PullUp_EN;
    gpio_config(&led_2);

    // Setting GPIO outputs
    GPIO_OUTPUT_SET(INDICATION_LED_1, 1);
    GPIO_OUTPUT_SET(INDICATION_LED_2, 0);
    vTaskDelay(LIVE_INDICATOR_DELAY);
    
    while (TRUE) {
        int counter = SLOW_PULSE_FREQ;

        // Slow Red LED pulse, WiFi connecting
        while (connection_status != CONNECTION_SUCCESS) {
            GPIO_OUTPUT_SET(INDICATION_LED_1, counter / SLOW_PULSE_FREQ);
            vTaskDelay(1);
            counter = (counter + 1) % (SLOW_PULSE_FREQ * 2);
            live_indication_reset = 0;
        }

        counter = QUICK_PULSE_FREQ;

        // Fast Red LED pulse, connection and NTP update
        while ((connection_status == CONNECTION_SUCCESS) && (ntp_status != CONNECTION_SUCCESS)) {
            GPIO_OUTPUT_SET(INDICATION_LED_1, counter / QUICK_PULSE_FREQ);
            vTaskDelay(1);
            counter = (counter + 1) % (QUICK_PULSE_FREQ * 2);
            live_indication_reset = 0;
        }

        // Turn off Red LED
        GPIO_OUTPUT_SET(INDICATION_LED_1, 0);

        counter = QUICK_PULSE_FREQ;

        // Fast Green LED pulse, MQTT publishing
        while (mqtt_status == MQTT_PUBLISHING) {
            GPIO_OUTPUT_SET(INDICATION_LED_2, counter / QUICK_PULSE_FREQ);
            vTaskDelay(1);
            counter = (counter + 1) % (QUICK_PULSE_FREQ * 2);
            live_indication_reset = 0;
        }

        // Turn off Green LED
        GPIO_OUTPUT_SET(INDICATION_LED_2, 0);

        live_indication_reset = 0; // Watchdog reset

        vTaskDelay(QUICK_PULSE_FREQ);
    }

    vTaskDelete(NULL);
}

/**
 * This thread creates a fake MQTT traffic to check the integrity of the system. It
 * adds a text string every x times. Also dequeues any messages on subscribe queue
 * and prints it.
 * @param none
 * @return none
 */
void fake_mqtt_traffic() {
    printf("fake_mqtt_traffic starting...\n\0");
    vTaskDelay(1000);
    
    while(TRUE) {
        fake_publish("lihini/income\0");
        vTaskDelay(10000); // Every x times
    }
    
    vTaskDelete(NULL);
}

/**
 * Timed interrupt.
 * NOTE: This is for polling sensor data. Need to be implemented
 * @param xTimerHandle interrupt_timer
 * @return none
 */
void timed_interrupt_callback( xTimerHandle interrupt_timer ) {
    // Polling code here
}

/**
 * This function creates the timed interrupt that polls sensor data.
 * @param none
 * @return none
 */
void create_timed_interrupt() {
    // Create the timer to be used for interrupts
    xTimerHandle interrupt_timer = xTimerCreate(
        "poll_timer",
        1000/portTICK_RATE_MS,
        pdTRUE,
        NULL,
        timed_interrupt_callback
    );

    xTimerStart(interrupt_timer, 0); // Start
}
