#pragma once

#include "iotconnect_lib.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    UNDEFINED,
    MQTT_CONNECTED,
    MQTT_DISCONNECTED,
    MQTT_FAILED,
    // TODO: Sync statuses etc.
} IOT_CONNECT_STATUS;


typedef void (*IOT_CONNECT_STATUS_CB)(IOT_CONNECT_STATUS data);

typedef struct {

    char *env;    // Environment name. Contact your representative for details.
    char *cpid;   // Settings -> Company Profile.
    char *duid;   // Name of the device.
    wiced_mqtt_security_t security; // mqtt security structure with ca_cert, client cert and private key can be freed once connected
    IOTCL_OTA_CALLBACK ota_cb; // callback for OTA events.
    IOTCL_COMMAND_CALLBACK cmd_cb; // callback for command events.
    IOTCL_MESSAGE_CALLBACK msg_cb; // callback for ALL messages, including the specific ones like cmd or ota callback.
    IOT_CONNECT_STATUS_CB status_cb; // callback for connection status
} IOTCONNECT_CLIENT_CONFIG;

IOTCONNECT_CLIENT_CONFIG *IotConnectSdk_InitAndGetConfig();

wiced_result_t IotConnectSdk_Init();

bool IotConnectSdk_IsConnected();

IOTCL_CONFIG *IotConnectSdk_GetLibConfig();

void IotConnectSdk_SendPacket(const char *data);
void IotConnectSdk_SendDataPacket(uint8_t *data, size_t len);

void IotConnectSdk_Loop();

void IotConnectSdk_Disconnect();

#ifdef __cplusplus
}
#endif
