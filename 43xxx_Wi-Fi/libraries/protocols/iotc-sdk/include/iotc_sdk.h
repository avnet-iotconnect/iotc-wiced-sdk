#pragma once

#include <wiced.h>
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
} IOT_CONNECT_STATUS;

typedef void (*IOT_CONNECT_STATUS_CB)(IOT_CONNECT_STATUS status, void* event_data);

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
    IOTCL_OTA_CALLBACK ota_cb; // callback for OTA events.
    IOTCL_COMMAND_CALLBACK cmd_cb; // callback for command events.
    IOTCL_MESSAGE_CALLBACK msg_cb; // callback for ALL messages, including the specific ones like cmd or ota callback.
    IOT_CONNECT_STATUS_CB status_cb; // callback for connection status
} IOTCONNECT_CLIENT_CONFIG;

IOTCONNECT_CLIENT_CONFIG *IotConnectSdk_InitAndGetConfig();

wiced_result_t IotConnectSdk_Init();

bool IotConnectSdk_IsConnected();

IOTCL_CONFIG *IotConnectSdk_GetLibConfig();

wiced_result_t IotConnectSdk_SendPacket(const char *data);
wiced_result_t IotConnectSdk_SendDataPacket(uint8_t *data, size_t len);

void IotConnectSdk_Loop();

void IotConnectSdk_Disconnect();

#ifdef __cplusplus
}
#endif
