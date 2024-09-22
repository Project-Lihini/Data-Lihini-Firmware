/*
 * Project Name: Project Lihini
 * File Name: mqtt_conn.c
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

#include "mqtt_conn.h"

// Constants ----------------------------------------------------------

#define MQTT_BUFF_SIZE 100 // MQTT buffer size (Use 100)
#define MQTT_VERSION 3 // MQTT version (Use 3)
#define MQTT_SUBSCRIBE_RETRY_FREQ 100 // Frequency to retry subscribe
#define TIMESTAMP_SIZE 20 // Timestamp size (20 for DD-MM-YYYY hh:mm:ss)
#define SNTP_EPOCH_THRESHOLD 905536800 // Value after which NTP update is assumed to
// be successful

// --------------------------------------------------------------------

// Paho Variables -----------------------------------------------------

// These variables are created as per the requirement of Paho MQTT

MQTTClient client = DefaultClient;
unsigned char mqtt_buf[MQTT_BUFF_SIZE] = {0};
unsigned char mqtt_readbuf[MQTT_BUFF_SIZE] = {0};
struct Network network = {0};
MQTTPacket_connectData data = MQTTPacket_connectData_initializer;

// --------------------------------------------------------------------

// MQTT Variables -----------------------------------------------------

// Message ID of the last MQTT message. Used to stop receiving
// duplicate messages
uint16 last_mqtt_message = 65535;

ulong counter = 0; // Counted used by fake publish function

// --------------------------------------------------------------------

// External Variables -------------------------------------------------

// Main queues
extern xQueueHandle incoming_queue;         // Incoming from server
extern xQueueHandle outgoing_queue;         // Outgoing to server

extern int8 mqtt_status;                    // MQTT status

extern char unique_identifier[22];          //Unique identifier of the ESP32
// NOTE: This is used as the client ID for MQTT

// --------------------------------------------------------------------

/**
 * Initialize the MQTT queue. Both incoming and outgoing queues are initialized.
 * 
 * NOTE: The max_size defines what the maximum number of messages to be stored
 * in the queue. When more data are available, last message is dequeued and
 * new message is queued. Increasing the queue size may affect the function
 * of the program but shorter queue may result in data loss in case of conn.
 * loss.
 * 
 * @param int max_size Maximum size of the queues
 * @return none
 */
void mqtt_queue_init(int max_size) {
    incoming_queue = xQueueCreate(max_size, sizeof(struct QueueData));
    outgoing_queue = xQueueCreate(max_size, sizeof(struct QueueData));

    printf("Queues initialized.\n");
}

/**
 * Function to be called when MQTT functionality is required. Will handle
 * required processing to enable MQTT comm. Will return the success of
 * connection as defined in MQTT_CONNECTION_STATUS struct in wifi_conn.h.
 *      MQTT_CONNECTION_SUCCESS - Success
 *      MQTT_NETWORK_ERROR - Network error
 *      MQTT_CONNECTION_ERROR - Failed to connect to MQTT server
 *      MQTT_ACTIVE - MQTT connection is currently being used (Will go
 *                      into this state to indicate that a certain thread
 *                      is using the MQTT connection)
 *      MQTT_PUBLISHING - MQTT thread is publishing some data
 *      MQTT_PUBLISH_FAIL - Failed to publish
 *      MQTT_DISCONNECT - MQTT disconnected
 * @param char *mqtt_host MQTT host URL
 * @param char *mqtt_client_id Client ID of the device
 * @param int mqtt_port MQTT port (502 default)
 * @param int mqtt_timeout MQTT connecting timeout
 * @param int mqtt_buff_length Buffer length
 * @return int Success/Fail
 */
uint8 mqtt_connect(char *mqtt_host, char *mqtt_client_id, int mqtt_port, int mqtt_timeout, int mqtt_buff_length) {
    int error = MQTT_DISCONNECT;

    // Connect to network
    NewNetwork(&network);
    error = ConnectNetwork(&network, mqtt_host, mqtt_port);

    if (error == 0) {
        // If connected, new MQTT client is created
        NewMQTTClient(&client, &network, mqtt_timeout, mqtt_buf, mqtt_buff_length, mqtt_readbuf, mqtt_buff_length);
        
        data.willFlag = 0;
        data.MQTTVersion = MQTT_VERSION;
        data.clientID.cstring = mqtt_client_id;
        data.username.cstring = MQTT_USERNAME;
        data.password.cstring = MQTT_PASSWORD;
        data.keepAliveInterval = MQTT_KEEP_ALIVE_TIME;
        data.cleansession = 0;

        // Connect
        error = MQTTConnect(&client, &data);

        if (error == 0) {
            // MQTT connection success
            printf("MQTT Connected.\n");
            return MQTT_CONNECTION_SUCCESS;
        } else {
            // MQTT connection fail
            printf("Failed to connect to MQTT client.\n");
            return MQTT_CONNECTION_ERROR;
        }
    } else {
        // Network connection success
        printf("Failed to connect to MQTT network.\n");
        return MQTT_NETWORK_ERROR;
    }

    return MQTT_DISCONNECT;
}

/**
 * Disconnects the MQTT network. Will be called after MQTT function is
 * completed. This is to stop traffic congestion in the MQTT server.
 * Always disconect once all traffic is completed.
 * @param none
 * @return none
 */
void mqtt_disconnect() {
    DisconnectNetwork(&network);
}

/**
 * Checks the topic for available messages. If the server is to congested, the
 * subscription tend to get expired. This function call ensures that this
 * won't affect operation. Also the connection is disconnected everytime
 * work is completed. Returns error state as defined in MQTT_MESSAGE_STATUS
 * by mqtt_conn.h.
 *      MQTT_MESSAGE_SUCCESS - Subscribe success
 *      MQTT_TOPIC_LENGTH_EXCEEDED - Topic length too long
 *      MQTT_BUFFER_LENGTH_EXCEED - Message length too long
 *      MQTT_PUBLISH_ERROR - Publish error
 * @param char *mqtt_topic Topic to subscribe
 * @param int qos_state QoS state to receive messages
 * @return int Success/Fail
 */
uint8 mqtt_check_topic(char *mqtt_topic, int qos_state) {
    // Check for connection
    if (client.isconnected) {
        // Check for topic length exceed
        if (strlen(mqtt_topic) <= MAX_MQTT_TOPIC_SIZE) {
            int ret = -1;

            while (TRUE) {
                // Unsubscribe
                ret = MQTTUnsubscribe(&client, mqtt_topic);
                //printf("MQTT Unsubscribed. Err: %d...", ret);

                // Subscribe
                ret = MQTTSubscribe(&client, mqtt_topic, qos_state, topic_received);
                //printf("MQTT Subscribed. Err: %d\n", ret);

                // QoS state tally with sent QoS, (Indicate success) then exit loop
                if (ret == qos_state)
                    break;

                vTaskDelay(MQTT_SUBSCRIBE_RETRY_FREQ);
            }

            vTaskDelay(MQTT_PUBLISH_TIMEOUT);

            return MQTT_MESSAGE_SUCCESS;
        } else {
            // Topic length exceed
            printf("MQTT topic length exceeded.\n");
            return MQTT_TOPIC_LENGTH_EXCEEDED;
        }
    } else {
        // Disconnected network
        printf("MQTT network disconnected.\n");
        return MQTT_CONNECTION_DISCONNECT;
    }

    return MQTT_CONNECTION_DISCONNECT;
}

/**
 * Callback when a message is received for a certain topic. Will copy the message
 * to the incoming queue.
 * @param MessageData* md Received message
 * @return none
 */
void ICACHE_FLASH_ATTR topic_received(MessageData* md) {
    // Check if last received message ID is not the current message. This stops
    // duplicates from entering the queue
    if (last_mqtt_message != md->message->id) {
        printf("Message received.\n");

        struct QueueData incoming_data = {0};

        // Copy topic and message to queue
        strncpy(incoming_data.topic, md->topic->lenstring.data, md->topic->lenstring.len);
        strcpy(incoming_data.payload, md->message->payload);

        printf("Topic: %s | Payload: %s\n", incoming_data.topic, incoming_data.payload);

        // Queueing the message
        if (xQueueSendToBack(incoming_queue, &incoming_data, 0) == pdPASS) {
            printf("Incoming data queued.\n");
        } else {
            printf("Failed to queue incoming data. Will drop the data.\n");
        }
    }

    last_mqtt_message = md->message->id; // Update message ID
}

/**
 * Enqueue data in the MQTT publish queue. Any function that needs to
 * publish any data to the MQTT server can call this function. The
 * message should be formatted in the struct QueueData. Will return
 * error state as defined in MQTT_QUEUE_STATUS by mqtt_conn.h.
 *      MQTT_QUEUE_SUCCESS - Queue success success
 *      MQTT_QUEUE_EXCEEDED - Queue size exceeded
 * @param struct QueueData data Data to be queued
 * @return int Success/Fail
 */
uint8 mqtt_enqueue(struct QueueData data) {
    int error = MQTT_QUEUE_SUCCESS; // Error state
    int free_ram = xPortGetFreeHeapSize(); // Heap size
    uint32_t queue_size = uxQueueMessagesWaiting(outgoing_queue); // Queue size

    // Check both free_ram and queue_size before queueing. If queue
    // is filled or RAM availabe is lower than threshold, last
    // message is dequeued.
    if ((free_ram < RAM_THRESHOLD) || (queue_size >= MAX_QUEUE_SIZE)) {
        printf("RAM: %d B / %d B | Queue: %d / %d\n", free_ram, TOTAL_RAM, queue_size, MAX_QUEUE_SIZE);
        printf("Queue/RAM exceeded. Dequeuing...\n");

        struct QueueData publish_sink = {0};
        xQueueReceive(outgoing_queue, &publish_sink, 0);

        error = MQTT_QUEUE_EXCEEDED;
    }

    xQueueSendToBack(outgoing_queue, &data, 0); // Queue message

    return error;
}

/**
 * Calls mqtt_publish() to publish the messages in the MQTT incoming queue.
 * This function must be called after MQTT is connected. Returns error
 * state as defined in MQTT_QUEUE_STATUS by mqtt_conn.h.
 *      MQTT_QUEUE_SUCCESS - Queue success success
 *      MQTT_QUEUE_FAIL - At least 1 message failed to publish
 *      MQTT_CONNECTION_DISCONNECT - Disconnected
 * @param none
 * @return int Success/Fail
 */
uint8 mqtt_queue_publish() {
    // Check if connected
    if (client.isconnected) {
        int error_count = 0; // Will hold the no of failed publishes

        printf("Ready to publish %d messages in queue...\n", uxQueueMessagesWaiting(outgoing_queue));

        // While the queue still has messages
        while (uxQueueMessagesWaiting(outgoing_queue) > 0) {
            mqtt_status = MQTT_PUBLISHING; // Status set to prevent conflicts

            int retry = 0;
            uint8 mqtt_error = MQTT_MESSAGE_SUCCESS;

            vTaskDelay(MQTT_PUBLISH_TIMEOUT);

            // Check retry count
            while (retry < MAX_RETRY_COUNT) {
                struct QueueData publish_data = {0};

                // Retrieve data
                if (xQueuePeek(outgoing_queue, &publish_data, 0) != pdPASS) {
                    printf("Failed to read publish queue.\n");
                    continue;
                }

                // Publish
                mqtt_error = mqtt_publish(publish_data.payload, publish_data.topic, strlen(publish_data.payload), QOS1, 0);

                if (mqtt_error != MQTT_PUBLISH_ERROR) {
                    // If successful, dequeue message
                    if (xQueueReceive(outgoing_queue, &publish_data, 0) != pdPASS) {
                        /* Process received data */
                        printf("Failed to dequeue publish queue.\n");
                    }
                    
                    break;
                } else {
                    // Retry otherwise
                    printf("Retrying...\n");
                    retry++;
                }
            }

            if (mqtt_error == MQTT_PUBLISH_ERROR) {
                // Publish error (Will break because if this timed out
                // we know the others will not and must not be looped again)
                printf("Publishing timed out. Giving up. Will try again...\n");
                error_count++;
                break;
            } else if (mqtt_error != MQTT_MESSAGE_SUCCESS) {
                // Publish success
                error_count++;
            }
        }

        if (error_count > 0) {
            // At least one publish failed
            printf("Publishing failed in one or more instances.\n");
            return MQTT_QUEUE_FAIL;
        } else {
            // Publish success
            printf("MQTT publish queue cleared.\n");
            return MQTT_QUEUE_SUCCESS;
        }
    } else {
        // Disconnected network
        printf("MQTT network disconnected.\n");
        return MQTT_CONNECTION_DISCONNECT;
    }

    return MQTT_CONNECTION_DISCONNECT;
}

/**
 * Publishes MQTT messages to the server. This function must be called after
 * MQTT is connected. Returns error state as defined in MQTT_MESSAGE_STATUS
 * by mqtt_conn.h.
 *      MQTT_MESSAGE_SUCCESS - Publish success
 *      MQTT_TOPIC_LENGTH_EXCEEDED - Topic length too long
 *      MQTT_BUFFER_LENGTH_EXCEED - Message length too long
 *      MQTT_PUBLISH_ERROR - Publish error
 * @param char *mqtt_message MQTT message to be published
 * @param char *mqtt_topic Topic
 * @param uint16 mqtt_message_size Message size
 * @param enum QoS qos_state QoS state
 * @param uint8 retained Retain message in server or not
 * @param int qos_state QoS state to receive messages
 * @return int Success/Fail
 */
uint8 mqtt_publish(char *mqtt_message, char *mqtt_topic, uint16 mqtt_message_size, enum QoS qos_state, uint8 retained) {
    // Check the topic size
    if (strlen(mqtt_topic) <= MAX_MQTT_TOPIC_SIZE) {
        // Check the message size
        if (mqtt_message_size <= MAX_MQTT_PAYLOAD) {
            // Payload created
            MQTTMessage message = {
                .payload = mqtt_message,
                .payloadlen = mqtt_message_size,
                .dup = 0,
                .qos = qos_state,
                .retained = retained
            };

            // Publish
            int error = MQTTPublish(&client, mqtt_topic, &message);

            if (error == 0) {
                // Publish successful
                printf("MQTT message published successfully.\n");
                return MQTT_MESSAGE_SUCCESS;
            } else {
                // Publish fail
                printf("Failed to publish to MQTT client.\n");
                return MQTT_PUBLISH_ERROR;
            }
        } else {
            //Message size error
            printf("MQTT message size exceeded.\n");
            return MQTT_BUFFER_LENGTH_EXCEED;
        }
    } else {
        // Topic length error
        printf("MQTT topic length exceeded.\n");
        return MQTT_TOPIC_LENGTH_EXCEEDED;
    }

    return MQTT_CONNECTION_DISCONNECT;
}

/**
 * Fake publish for fake traffic. It adds a text string every x times.
 * Also dequeues any messages on subscribe queue and prints it.
 * FOR TESTING PURPOSES ONLY
 * @param char *topic Publish topic
 * @return none
 */
void fake_publish(char *topic) {
    u_long current_time = get_time();

    // Check if valid time is available
    if (current_time > SNTP_EPOCH_THRESHOLD) {
        struct QueueData outgoing_data = {0};
        char data[MAX_MQTT_PAYLOAD] = {0};
        char timestamp[TIMESTAMP_SIZE] = {0};

        // Get current time in string format: DD-MM-YYYY hh:mm:ss
        time_to_str(&timestamp, current_time);

        // Create a dummy MQTT message in format:
        // [DD-MM-YYYY hh:mm:ss ESP says: X]
        // X increase after every publish
        sprintf(data, "[%s] %s says: %d\0", timestamp, unique_identifier, counter);

        // Topic and message set
        strcpy(outgoing_data.topic, topic);
        strcpy(outgoing_data.payload, data);

        while (TRUE) {
            // While publishing didn't succeed, try to enqueue the
            // MQTT message. Will not enqueue until mqtt_status is
            // MQTT_ACTIVE or MQTT_PUBLISHING
            if (!((mqtt_status == MQTT_ACTIVE) || (mqtt_status == MQTT_PUBLISHING))) {
                mqtt_enqueue(outgoing_data);
                break;
            }
        }

        // Check subscribe queue has messages and print
        while (uxQueueMessagesWaiting(incoming_queue) > 0) {
            struct QueueData incoming_data = {0};

            xQueueReceive(incoming_queue, &incoming_data, 0);
            printf("Server Says: [Topic: %s | Payload: %s]\n", incoming_data.topic, incoming_data.payload);
        }

        counter++;
    }
}
