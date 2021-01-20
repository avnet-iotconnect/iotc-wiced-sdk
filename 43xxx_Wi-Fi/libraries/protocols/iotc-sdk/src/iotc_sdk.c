
#include "iotconnect_discovery.h"
#include "iotconnect_event.h"

#include "iotc_wiced_discovery.h"
#include "iotc_wiced_mqtt.h"
#include "iotc_sdk.h"
#include "iotconnect_client_config.h"


IOTCL_SyncResponse *sync_response = NULL;

static IOTCONNECT_CLIENT_CONFIG config;
static IOTCL_CONFIG lib_config;
static IOTCONNECT_MQTT_CONFIG mqtt_config;

static void report_sync_error(IOTCL_SyncResponse *response) {
    if (NULL == response) {
        WPRINT_LIB_ERROR(("IOTC_SyncResponse is NULL. Out of memory?\n"));
        return;
    }
    switch (response->ds) {
        case IOTCL_SR_DEVICE_NOT_REGISTERED:
            WPRINT_LIB_ERROR(("IOTC_SyncResponse error: Not registered\n"));
            break;
        case IOTCL_SR_AUTO_REGISTER:
            WPRINT_LIB_ERROR(("IOTC_SyncResponse error: Auto Register\n"));
            break;
        case IOTCL_SR_DEVICE_NOT_FOUND:
            WPRINT_LIB_ERROR(("IOTC_SyncResponse error: Device not found\n"));
            break;
        case IOTCL_SR_DEVICE_INACTIVE:
            WPRINT_LIB_ERROR(("IOTC_SyncResponse error: Device inactive\n"));
            break;
        case IOTCL_SR_DEVICE_MOVED:
            WPRINT_LIB_ERROR(("IOTC_SyncResponse error: Device moved\n"));
            break;
        case IOTCL_SR_CPID_NOT_FOUND:
            WPRINT_LIB_ERROR(("IOTC_SyncResponse error: CPID not found\n"));
            break;
        case IOTCL_SR_UNKNOWN_DEVICE_STATUS:
            WPRINT_LIB_ERROR(("IOTC_SyncResponse error: Unknown device status error from server\n"));
            break;
        case IOTCL_SR_ALLOCATION_ERROR:
            WPRINT_LIB_ERROR(("IOTC_SyncResponse internal error: Allocation Error\n"));
            break;
        case IOTCL_SR_PARSING_ERROR:
            WPRINT_LIB_ERROR(
                    ("IOTC_SyncResponse internal error: Parsing error. Please check parameters passed to the request.\n"));
            break;
        default:
            WPRINT_LIB_INFO(("WARN: report_sync_error called, but no error returned?\n"));
            break;
    }
}

static void on_iotconnect_status(IOT_CONNECT_STATUS status) {
    if (config.status_cb) {
        config.status_cb(status);
    }
}

void IotConnectSdk_Disconnect() {
    WPRINT_LIB_INFO(("Disconnecting...\n"));
    IOTCL_DiscoveryFreeSyncResponse(sync_response);
    sync_response = NULL;
    iotc_wiced_mqtt_disconnect();
    iotc_wiced_mqtt_deinit();
}

void IotConnectSdk_SendPacket(const char *data) {
    if (0 != iotc_wiced_mqtt_publish((uint8_t *) data, strlen(data))) {
        WPRINT_LIB_ERROR(("\n\t Device_Attributes_Data Publish failure"));
    }
}

void IotConnectSdk_SendDataPacket(uint8_t *data, size_t len) {
    if (0 != iotc_wiced_mqtt_publish(data, len)) {
        WPRINT_LIB_ERROR(("\n\t Device_Attributes_Data Publish failure"));
    }
}

static void on_message_intercept(IOTCL_EVENT_DATA data, IotConnectEventType type) {
    switch (type) {
        case ON_FORCE_SYNC:
            IotConnectSdk_Disconnect();
            sync_response = iotc_wiced_discover(config.env, config.cpid, config.duid);
            if (!sync_response || sync_response->ds != IOTCL_SR_OK) {
                report_sync_error(sync_response);
                IOTCL_DiscoveryFreeSyncResponse(sync_response);
                sync_response = NULL;
                return;
            }
            (void) iotc_wiced_mqtt_init(&mqtt_config, &config.security);
            break;
        case ON_CLOSE:
            WPRINT_LIB_INFO(("Got a disconnect request. Closing the mqtt connection. Device restart is required.\n"));
            IotConnectSdk_Disconnect();
            break;
        default:
            break; // not handling nay other messages
    }

    if (NULL != config.msg_cb) {
        config.msg_cb(data, type);
    }
}

IOTCL_CONFIG *IotConnectSdk_GetLibConfig() {
    return IOTCL_GetConfig();
}

IOTCONNECT_CLIENT_CONFIG *IotConnectSdk_InitAndGetConfig() {
    memset(&config, 0, sizeof(config));
    return &config;
}

bool IotConnectSdk_IsConnected() {
    return iotc_wiced_mqtt_is_connected();
}

void iotc_on_mqtt_data(const uint8_t *data, size_t len, const uint8_t *topic, const uint32_t topic_len) {
    (void) topic; // ignore topic for now
    (void) topic_len;
    char *str = malloc(len + 1);
    memcpy(str, data, len);
    str[len] = 0;
    WPRINT_LIB_INFO(("event>>> %s\n", str));
    if (!IOTCL_ProcessEvent(str)) {
        WPRINT_LIB_ERROR(("Error encountered while processing %s\n", str));
    }
    free(str);
}

///////////////////////////////////////////////////////////////////////////////////
// this the Initialization os IoTConnect SDK
wiced_result_t IotConnectSdk_Init() {
    wiced_result_t ret;

    iotc_wiced_discovery_init();
    IOTCL_SyncResponse *sync_response = iotc_wiced_discover(config.env, config.cpid, config.duid);
    iotc_wiced_discovery_deinit();

    if (!sync_response || sync_response->ds != IOTCL_SR_OK) {
        report_sync_error(sync_response);
        IOTCL_DiscoveryFreeSyncResponse(sync_response);
        sync_response = NULL;
        return WICED_ERROR;
    }

    // We want to print only first 4 characters of cpid. %.4s doesn't seem to work with prink
    char cpid_buff[5];
    strncpy(cpid_buff, sync_response->cpid, 4);
    cpid_buff[4] = 0;
    WPRINT_LIB_INFO(("CPID: %s***\n", cpid_buff));
    WPRINT_LIB_INFO(("ENV:  %s\n", config.env));

    memset(&mqtt_config, 0, sizeof(mqtt_config));
    mqtt_config.sr = sync_response;
    mqtt_config.data_cb = iotc_on_mqtt_data;
    mqtt_config.status_cb = on_iotconnect_status;

    ret = iotc_wiced_mqtt_init(&mqtt_config, &config.security);
    if (WICED_SUCCESS != ret) {
        return ret;
    }

    lib_config.device.env = config.env;
    lib_config.device.cpid = config.cpid;
    lib_config.device.duid = config.duid;
    lib_config.telemetry.dtg = sync_response->dtg;;
    lib_config.event_functions.ota_cb = config.ota_cb;
    lib_config.event_functions.cmd_cb = config.cmd_cb;

    // intercept internal processing and forward to client
    lib_config.event_functions.msg_cb = on_message_intercept;

    if (!IOTCL_Init(&lib_config)) {
        WPRINT_LIB_ERROR(("Failed to initialize the IoTConnect Lib\n"));
    }

    return 0;
}
