/*
 * Project Name: Project Lihini
 * File Name: app_conf.h
 * Author: Asanka Sovis
 * Created: 19/09/2024
 * Description: All configs related to the project.
 *
 * Modified By: Asanka Sovis
 * Modified: 22/09/2024
 * 
 * Changelog:
 *   - Initial Commit
 * 
 * Copyright (C) 2024 Project Lihini. All rights reserved.
 */

// WiFi -------------------------------------------------------------------------------------

#define DEFAULT_WIFI_SSID "SSID2"                   // Default WiFi SSID
#define DEFAULT_WIFI_PASSWORD "!@#$%%12345"         // Default WiFi password

// Connection -------------------------------------------------------------------------------
#define DEFAULT_CONN_SERVER "www.google.com"        // Default ping server

// SNTP -------------------------------------------------------------------------------------

#define DEFAULT_SNTP_SERVER "time.nist.gov"         // Default SNTP server
#define DEFAULT_TIMEZONE kAtcZoneAsia_Colombo       // Default timezone
#define SNTP_UPDATE_TIMEOUT 500                     // SNTP update timeout (in msec)

// MQTT Conn -------------------------------------------------------------------------------

#define DEFAULT_MQTT_SERVER "test.mosquitto.org"    // Default MQTT server
#define MQTT_PORT 1883                              // MQTT port
#define MQTT_TIMEOUT 5000                           // MQTT update timeout (in msec)
#define MQTT_BUFF_SIZE 100                          // MQTT buffer size
#define MQTT_USERNAME ""                            // MQTT server uname
#define MQTT_PASSWORD ""                            // MQTT server pword
#define MQTT_KEEP_ALIVE_TIME 10                     // MQTT keep-alive time (in secs)

// MQTT Subscribe/ Publish -----------------------------------------------------------------

#define MQTT_SUBSCRIBE_TOPIC "lihini/outgo"         // MQTT subscribe topic
#define MQTT_PUBLISH_TIMEOUT 10                     // MQTT publish timeout (in msec)
#define MAX_RETRY_COUNT 3                           // MQTT retry count (Connect, Publish, Subscribe)

// MQTT payload/ queue ---------------------------------------------------------------------

#define MAX_QUEUE_SIZE 50                           // Maximum queue size
#define MAX_MQTT_TOPIC_SIZE 50                      // Maximum topic size
#define MAX_MQTT_PAYLOAD 150                        // Maximum MQTT payload

// Indicators ------------------------------------------------------------------------------

#define INDICATION_LED_1 14                         // RED LED GPIO
#define INDICATION_LED_2 12                         // Green LED GPIO

// RAM -------------------------------------------------------------------------------------

#define TOTAL_RAM 16000                             // Total RAM in device (in bytes)
#define RAM_THRESHOLD 200                           // Threshold RAM for queue (in bytes)

// -----------------------------------------------------------------------------------------
