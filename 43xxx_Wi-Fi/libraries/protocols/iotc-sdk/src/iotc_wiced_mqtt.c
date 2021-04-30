
/*
 * Copyright 2020, Cypress Semiconductor Corporation or a subsidiary of
 * Cypress Semiconductor Corporation. All Rights Reserved.
 *
 * This software, associated documentation and materials ("Software"),
 * is owned by Cypress Semiconductor Corporation
 * or one of its subsidiaries ("Cypress") and is protected by and subject to
 * worldwide patent protection (United States and foreign),
 * United States copyright laws and international treaty provisions.
 * Therefore, you may use this Software only as provided in the license
 * agreement accompanying the software package from which you
 * obtained this Software ("EULA").
 * If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
 * non-transferable license to copy, modify, and compile the Software
 * source code solely for use in connection with Cypress's
 * integrated circuit products. Any reproduction, modification, translation,
 * compilation, or representation of this Software except as specified
 * above is prohibited without the express written permission of Cypress.
 *
 * Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
 * reserves the right to make changes to the Software without notice. Cypress
 * does not assume any liability arising out of the application or use of the
 * Software or any product or circuit described in the Software. Cypress does
 * not authorize its products for use in any products where a malfunction or
 * failure of the Cypress product may reasonably be expected to result in
 * significant property damage, injury or death ("High Risk Product"). By
 * including Cypress's product in a High Risk Product, the manufacturer
 * of such system or application assumes all risk of such use and in doing
 * so agrees to indemnify Cypress against all liability.
 */

//
// Copyright: Avnet 2020
// Modified by Nikola Markovic <nikola.markovic@avnet.com> on 1/18/2021.
//

#include <stddef.h>
#include <stdbool.h>

#include <wiced.h>
#include <mqtt_api.h>
#include "iotc_wiced_mqtt.h"

#define DEFAULT_MQTT_TIMEOUT_MS 10000

#ifndef IOTC_SDK_RESOLVE_TIMEOUT_MS
#define IOTC_SDK_RESOLVE_TIMEOUT_MS 10000
#endif

#ifndef IOTC_SDK_KEEPALIVE_INTERVAL_SECS
#define IOTC_SDK_KEEPALIVE_INTERVAL_SECS 30
#endif

static wiced_result_t mqtt_connection_event_cb(wiced_mqtt_object_t mqtt_object, wiced_mqtt_event_info_t *event);

static wiced_result_t mqtt_wait_for(wiced_mqtt_event_type_t event, uint32_t timeout);

wiced_result_t mqtt_conn_open(
        const char *client_id,
        const char *username,
        const char *password,
        int keepalive_secs,
        wiced_mqtt_object_t mqtt_obj,
        wiced_ip_address_t *address,
        wiced_interface_t interface,
        wiced_mqtt_callback_t callback,
        wiced_mqtt_security_t *security
);

static wiced_result_t mqtt_sdk_subscribe(wiced_mqtt_object_t mqtt_obj, char *topic, uint8_t qos);

static wiced_result_t mqtt_sdk_unsubscribe(wiced_mqtt_object_t mqtt_obj, char *topic);

static wiced_result_t
mqtt_sdk_publish(wiced_mqtt_object_t mqtt_obj, uint8_t qos, char *topic, uint8_t *data, uint32_t data_len);


static wiced_mqtt_object_t mqtt_object;
static IotconnectMqttConfig *config;
static bool is_connected = false;

static wiced_ip_address_t broker_address;
static wiced_mqtt_event_type_t expected_event;
static wiced_semaphore_t semaphore;


/*
 * Callback function to handle connection events.
 */
static wiced_result_t mqtt_connection_event_cb(wiced_mqtt_object_t mqtt_object, wiced_mqtt_event_info_t *event) {
    //WPRINT_LIB_INFO(("[MQTT]: event: %d\n", event->type));

    switch (event->type) {
        case WICED_MQTT_EVENT_TYPE_CONNECT_REQ_STATUS:
            expected_event = event->type;
            wiced_rtos_set_semaphore(&semaphore);
            if (event->data.conn_ack.err_code != WICED_MQTT_CONN_ERR_CODE_NONE) {
                is_connected = true;
                WPRINT_LIB_INFO(("[MQTT] Connection Error code: %d\n", event->data.conn_ack.err_code));
                config->status_cb(MQTT_FAILED, NULL);
            } else {
                is_connected = false;
                config->status_cb(MQTT_CONNECTED, NULL);
            }
            break;
        case WICED_MQTT_EVENT_TYPE_DISCONNECTED:
            is_connected = false;
            config->status_cb(MQTT_DISCONNECTED, NULL);
            break;
        case WICED_MQTT_EVENT_TYPE_PUBLISHED:
            WPRINT_LIB_INFO(("[MQTT]: Packet ID %u acknowledged.\n", event->data.msgid));
            expected_event = event->type;
            config->status_cb(MQTT_PUBLISHED, &event->data.msgid);
            break;
        case WICED_MQTT_EVENT_TYPE_SUBSCRIBED:
            expected_event = event->type;
            wiced_rtos_set_semaphore(&semaphore);
            break;
        case WICED_MQTT_EVENT_TYPE_PUBLISH_MSG_RECEIVED: {
            wiced_mqtt_topic_msg_t msg = event->data.pub_recvd;
            WPRINT_LIB_INFO(
                    ("[MQTT] Received %.*s  for TOPIC : %.*s\n\n", (int) msg.data_len, msg.data, (int) msg.topic_len, msg.topic));
            config->data_cb(msg.data, msg.data_len, msg.topic, msg.topic_len);
            break;
        }
        case WICED_MQTT_EVENT_TYPE_UNSUBSCRIBED:
            WPRINT_LIB_INFO(("[MQTT]: Unsubscribed.\n"));
            break;
        default:
            break;
    }
    return WICED_SUCCESS;
}

static wiced_result_t mqtt_wait_for(wiced_mqtt_event_type_t event, uint32_t timeout) {
    if (wiced_rtos_get_semaphore(&semaphore, timeout) != WICED_SUCCESS) {
        return WICED_ERROR;
    } else {
        if (event != expected_event) {
            return WICED_ERROR;
        }
    }
    return WICED_SUCCESS;
}

/*
 * Open a connection and wait for config->mqtt_timeout_ms * 2 period to receive a connection open OK event
 */
wiced_result_t mqtt_conn_open(
        const char *client_id,
        const char *username,
        const char *password,
        int keepalive_secs,
        wiced_mqtt_object_t mqtt_obj,
        wiced_ip_address_t *address,
        wiced_interface_t interface,
        wiced_mqtt_callback_t callback,
        wiced_mqtt_security_t *security
) {
    wiced_mqtt_pkt_connect_t conninfo;
    wiced_result_t ret = WICED_SUCCESS;

    memset(&conninfo, 0, sizeof(conninfo));

    conninfo.port_number = 8883;                   /* set to 0 indicates library to use default settings */
    conninfo.mqtt_version = WICED_MQTT_PROTOCOL_VER4;
    conninfo.clean_session = 1;
    conninfo.client_id = (uint8_t *) client_id;
    conninfo.keep_alive = keepalive_secs;
    conninfo.password = (uint8_t *) password;
    conninfo.username = (uint8_t *) username;
    conninfo.peer_cn = (uint8_t *) "*.azure-devices.net";

    ret = wiced_mqtt_connect(mqtt_obj, address, interface, callback, security, &conninfo);
    if (ret != WICED_SUCCESS) {
        return WICED_ERROR;
    }
    if (mqtt_wait_for(WICED_MQTT_EVENT_TYPE_CONNECT_REQ_STATUS, config->mqtt_timeout_ms * 2) != WICED_SUCCESS) {
        return WICED_ERROR;
    }
    return WICED_SUCCESS;
}

/*
 * Subscribe to WICED_TOPIC and wait for 5 seconds to receive an ACM.
 */
static wiced_result_t mqtt_sdk_subscribe(wiced_mqtt_object_t mqtt_obj, char *topic, uint8_t qos) {
    wiced_mqtt_msgid_t pktid;
    pktid = wiced_mqtt_subscribe(mqtt_obj, topic, qos);
    if (pktid == 0) {
        return WICED_ERROR;
    }
    if (mqtt_wait_for(WICED_MQTT_EVENT_TYPE_SUBSCRIBED, config->mqtt_timeout_ms) != WICED_SUCCESS) {
        return WICED_ERROR;
    }
    return WICED_SUCCESS;
}

/*
 * Unsubscribe from WICED_TOPIC and wait for 10 seconds to receive an ACM.
 */
static wiced_result_t mqtt_sdk_unsubscribe(wiced_mqtt_object_t mqtt_obj, char *topic) {
    wiced_mqtt_msgid_t pktid;
    pktid = wiced_mqtt_unsubscribe(mqtt_obj, topic);

    if (pktid == 0) {
        WPRINT_LIB_INFO(("[MQTT]: Unable to subscribe subscribe\n"));
        return WICED_ERROR;
    }
    return WICED_SUCCESS;
}

/*
 * Publish (send) WICED_MESSAGE_STR to WICED_TOPIC and wait for 5 seconds to receive a PUBCOMP (as it is QoS=2).
 */
static wiced_mqtt_msgid_t
mqtt_sdk_publish(wiced_mqtt_object_t mqtt_obj, uint8_t qos, char *topic, uint8_t *data, uint32_t data_len) {
    wiced_mqtt_msgid_t pktid;
    pktid = wiced_mqtt_publish(mqtt_obj, topic, data, data_len, qos);

    if (pktid == 0) {
        WPRINT_LIB_INFO(("[MQTT]: Publish failed\n"));
    }
    return pktid;
}

wiced_result_t
iotc_wiced_mqtt_init(IotconnectMqttConfig *_config, wiced_mqtt_security_t *security) {
    wiced_result_t ret = WICED_SUCCESS;

    if (_config == NULL) {
        WPRINT_LIB_INFO(("[MQTT]: Missing configuration\n"));
        return WICED_BADARG;
    }
    config = _config;

    if (config->sr == NULL) {
        WPRINT_LIB_INFO(("[MQTT]: Missing sync response in config\n"));
        return WICED_BADARG;
    }
    if (config->data_cb == NULL || config->status_cb == NULL) {
        WPRINT_LIB_INFO(("[MQTT]: Callbacks are required in config\n"));
        return WICED_BADARG;
    }

    if (0 == config->mqtt_timeout_ms) {
        config->mqtt_timeout_ms = DEFAULT_MQTT_TIMEOUT_MS;
    }

    /* Memory allocated for mqtt object*/
    mqtt_object = (wiced_mqtt_object_t) malloc(WICED_MQTT_OBJECT_MEMORY_SIZE_REQUIREMENT);
    if (mqtt_object == NULL) {
        WPRINT_LIB_INFO(("[MQTT]: Don't have memory to allocate for mqtt object...\n"));
        return WICED_OUT_OF_HEAP_SPACE;
    }


    ret = wiced_hostname_lookup(config->sr->broker.host, &broker_address, IOTC_SDK_RESOLVE_TIMEOUT_MS,
                                WICED_STA_INTERFACE);
    if (ret == WICED_ERROR || broker_address.ip.v4 == 0) {
        WPRINT_LIB_INFO(("[MQTT] Error in resolving DNS\n"));
        return ret;
    }

    WPRINT_LIB_INFO(("[MQTT] Resolved Broker IP: %u.%u.%u.%u\n", (uint8_t)(GET_IPV4_ADDRESS(broker_address) >> 24),
            (uint8_t)(GET_IPV4_ADDRESS(broker_address) >> 16),
            (uint8_t)(GET_IPV4_ADDRESS(broker_address) >> 8),
            (uint8_t)(GET_IPV4_ADDRESS(broker_address) >> 0)));

    ret = wiced_mqtt_init(mqtt_object);
    if (ret != WICED_SUCCESS) {
        WPRINT_LIB_INFO(("[MQTT] Failed to init mqtt\n"));
        return ret;
    }

    wiced_rtos_init_semaphore(&semaphore);

    ret = mqtt_conn_open(
            config->sr->broker.client_id,
            config->sr->broker.user_name,
            config->sr->broker.pass,
            IOTC_SDK_KEEPALIVE_INTERVAL_SECS,
            mqtt_object,
            &broker_address,
            WICED_STA_INTERFACE,
            mqtt_connection_event_cb,
            security
    );

    if (WICED_SUCCESS != ret) {
        WPRINT_LIB_INFO(("[MQTT] Failed to initialize MQTT connection\n"));
        return ret;
    }

    ret = mqtt_sdk_subscribe(mqtt_object, config->sr->broker.sub_topic, WICED_MQTT_QOS_DELIVER_AT_LEAST_ONCE);
    if (WICED_SUCCESS != ret) {
        WPRINT_LIB_INFO(("[MQTT] Failed subscribe to devicebound messages\n"));
        return ret;
    }

    WPRINT_LIB_INFO(("[MQTT] Opening connection...\n"));
    return WICED_SUCCESS;
}

/*
 * Close a connection and wait for 5 seconds to receive a connection close OK event
 */
void iotc_wiced_mqtt_disconnect() {
    if (is_connected) {
        if (wiced_mqtt_disconnect(mqtt_object) != WICED_SUCCESS) {
            WPRINT_LIB_INFO(("[MQTT] Failed to disconnect\n"));
            return;
        }
    }
}

bool iotc_wiced_mqtt_is_connected() {
    return is_connected;
}

wiced_result_t iotc_wiced_mqtt_publish(const uint8_t *data, size_t len) {
    wiced_mqtt_msgid_t ret;
    ret = mqtt_sdk_publish(
            mqtt_object,
            WICED_MQTT_QOS_DELIVER_AT_LEAST_ONCE,
            config->sr->broker.pub_topic,
            (uint8_t *) data,
            len
    );
    return ret;
}

void iotc_wiced_mqtt_deinit() {
    wiced_result_t ret;

    wiced_rtos_deinit_semaphore(&semaphore);

    ret = mqtt_sdk_unsubscribe(mqtt_object, config->sr->broker.sub_topic);
    if (WICED_SUCCESS != ret) {
        WPRINT_LIB_INFO(("[MQTT] Failed to unsubscribe from devicebound topic\n"));
        return;
    }

    ret = wiced_mqtt_deinit(mqtt_object);
    if (WICED_SUCCESS != ret) {
        WPRINT_LIB_INFO(("[MQTT] Failed to deinitialize mqtt client\n"));
        return;
    }
    config = NULL;
}
