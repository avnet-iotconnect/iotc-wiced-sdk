//
// Copyright: Avnet 2020
// Created by Nikola Markovic <nikola.markovic@avnet.com> on 1/18/2021.
//

#pragma once

#include <stddef.h>
#include <mqtt_common.h>
#include "iotc_sdk.h"
#include "iotconnect_discovery.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*IOT_CONNECT_MQTT_ON_DATA_CB)(const uint8_t *data, size_t len, const uint8_t *topic,
                                            const uint32_t topic_len);

// This structure needs to be passed to iotc_wiced_mqtt_init and maintained in memory at all times during mqtt operation
typedef struct {
    IOTCL_SyncResponse *sr;
    uint32_t mqtt_timeout_ms; // Timeout for most operations. 2x timeout for connect and subscribe.
    IOT_CONNECT_MQTT_ON_DATA_CB data_cb; // callback for mqtt inbound messages
    IOT_CONNECT_STATUS_CB status_cb; // callback for nqtt status
} IOTCONNECT_MQTT_CONFIG;

wiced_result_t iotc_wiced_mqtt_init(IOTCONNECT_MQTT_CONFIG *_config, wiced_mqtt_security_t *security);

wiced_mqtt_msgid_t iotc_wiced_mqtt_publish(const uint8_t *data, size_t len);

void iotc_wiced_mqtt_disconnect();

bool iotc_wiced_mqtt_is_connected();

void iotc_wiced_mqtt_deinit();

#ifdef __cplusplus
}
#endif
