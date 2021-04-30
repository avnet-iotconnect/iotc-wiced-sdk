#pragma once

#include <wiced.h>
#include <mqtt_common.h>
#include "iotconnect_lib.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    UNDEFINED,
    MQTT_CONNECTED,
    MQTT_DISCONNECTED,
    MQTT_PUBLISHED,
    MQTT_FAILED,
    // TODO: Sync statuses etc.
} IotconnectConnectionStatus;

typedef void (*IotConnectStatusCallback)(IotconnectConnectionStatus status, void* event_data);

typedef struct {
    /* IoTConnect device connection parameters */
    char *env;    // Environment name. Contact your representative for details.
    char *cpid;   // Settings -> Company Profile.
    char *duid;   // Device ID.
    wiced_mqtt_security_t security; // mqtt security structure with ca_cert, client cert and private key can be freed once connected

    /* timing settings */
    uint32_t mqtt_timeout_ms; // Timeout for most operations. 2x timeout for connect and subscribe. Default: 10000
    int num_discovery_tires; // How many times to retry discovery Default: 3

    /* callbacks */
    IotclOtaCallback ota_cb; // callback for OTA events.
    IotclCommandCallback cmd_cb; // callback for command events.
    IotclMessageCallback msg_cb; // callback for ALL messages, including the specific ones like cmd or ota callback.
    IotConnectStatusCallback status_cb; // callback for connection status
} IotconnectClientConfig;

IotconnectClientConfig *iotconnect_sdk_init_and_get_config();

wiced_result_t iotconnect_sdk_init();

bool iotconnect_sdk_is_connected();

IotclConfig *iotconnect_sdk_get_lib_config();

wiced_result_t iotconnect_sdk_send_packet(const char *data);
wiced_result_t iotconnect_sdk_send_data_packet(uint8_t *data, size_t len);

void iotconnect_sdk_loop();

void iotconnect_sdk_disconnect();

#ifdef __cplusplus
}
#endif
