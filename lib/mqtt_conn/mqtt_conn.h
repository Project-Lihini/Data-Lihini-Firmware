/*
 * Project Name: Project Lihini
 * File Name: mqtt_conn.h
 * Author: Asanka Sovis
 * Created: 19/09/2024
 * Description: All functions related to MQTT including connection,
 * queuing, subscribing, publishing and disconnecting.
 *
 * Modified By: Asanka Sovis
 * Modified: 22/09/2024
 * 
 * Changelog:
 *   - Initial Commit
 * 
 * Reference:
 *   - https://github.com/baoshi/ESP-RTOS-Paho
 *
 * Copyright (C) 2024 Project Lihini. All rights reserved.
 */

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "paho/MQTTClient.h"
#include "paho/MQTTESP8266.h"
#include "../../include/app_conf.h"

// Struct to hold the MQTT data. Both topic and payload as per the
// requirement of MQTT client.
struct QueueData {
    char topic[MAX_MQTT_TOPIC_SIZE];
    char payload[MAX_MQTT_PAYLOAD];
};

// Type to hold the MQTT connection status
typedef enum {
    MQTT_CONNECTION_SUCCESS,    // Success
    MQTT_NETWORK_ERROR,         // Network error
    MQTT_CONNECTION_ERROR,      // Failed to connect to MQTT server
    MQTT_ACTIVE,                // QTT connection is currently being used
    MQTT_PUBLISHING,            // MQTT thread is publishing some data
    MQTT_PUBLISH_FAIL,          // Failed to publish
    MQTT_DISCONNECT             // MQTT disconnected
} MQTT_CONNECTION_STATUS;

// Type to hold the MQTT subscribe/publish status
typedef enum {
    MQTT_MESSAGE_SUCCESS,       // Success
    MQTT_TOPIC_LENGTH_EXCEEDED, // Topic length too long
    MQTT_BUFFER_LENGTH_EXCEED,  // Message length too long
    MQTT_PUBLISH_ERROR          // Publish error
} MQTT_MESSAGE_STATUS;

// Type to hold the MQTT queue publish status
typedef enum {
    MQTT_QUEUE_SUCCESS,         // Success
    MQTT_QUEUE_FAIL,            // At least 1 message failed to publish
    MQTT_CONNECTION_DISCONNECT, // Disconnected
    MQTT_QUEUE_EXCEEDED         // Queue size exceeded
} MQTT_QUEUE_STATUS;

void mqtt_queue_init(int max_size);
uint8 mqtt_connect(char *mqtt_host, char *mqtt_client_id, int mqtt_port, int mqtt_timeout, int mqtt_buff_length);
void mqtt_disconnect();
uint8 mqtt_check_topic(char *topic, int qos_state);
void ICACHE_FLASH_ATTR topic_received(MessageData* md);
uint8 mqtt_enqueue(struct QueueData data);
uint8 mqtt_queue_publish();
uint8 mqtt_publish(char *mqtt_message, char *mqtt_topic, uint16 mqtt_message_size, enum QoS qos_state, uint8 retained);
void fake_publish(char *topic);
