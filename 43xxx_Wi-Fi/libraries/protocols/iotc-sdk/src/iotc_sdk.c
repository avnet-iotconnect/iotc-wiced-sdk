
#include "iotconnect_discovery.h"
#include "iotconnect_event.h"

#include "iotc_wiced_discovery.h"
#include "iotc_wiced_mqtt.h"
#include "iotc_sdk.h"
#include "iotconnect_client_config.h"

#define IOTC_SDK_DEFAULT_NUM_DISCOVERY_TRIES 3


IotclSyncResponse *sync_response = NULL;

static IotconnectClientConfig config;
static IotclConfig lib_config;
static IotconnectMqttConfig mqtt_config;

static void report_sync_error(IotclSyncResponse *response) {
    if (NULL == response) {
        WPRINT_LIB_INFO(("IOTC_SyncResponse is NULL. Out of memory?\n"));
        return;
    }
    switch (response->ds) {
        case IOTCL_SR_DEVICE_NOT_REGISTERED:
            WPRINT_LIB_INFO(("IOTC_SyncResponse error: Not registered\n"));
            break;
        case IOTCL_SR_AUTO_REGISTER:
            WPRINT_LIB_INFO(("IOTC_SyncResponse error: Auto Register\n"));
            break;
        case IOTCL_SR_DEVICE_NOT_FOUND:
            WPRINT_LIB_INFO(("IOTC_SyncResponse error: Device not found\n"));
            break;
        case IOTCL_SR_DEVICE_INACTIVE:
            WPRINT_LIB_INFO(("IOTC_SyncResponse error: Device inactive\n"));
            break;
        case IOTCL_SR_DEVICE_MOVED:
            WPRINT_LIB_INFO(("IOTC_SyncResponse error: Device moved\n"));
            break;
        case IOTCL_SR_CPID_NOT_FOUND:
            WPRINT_LIB_INFO(("IOTC_SyncResponse error: CPID not found\n"));
            break;
        case IOTCL_SR_UNKNOWN_DEVICE_STATUS:
            WPRINT_LIB_INFO(("IOTC_SyncResponse error: Unknown device status error from server\n"));
            break;
        case IOTCL_SR_ALLOCATION_ERROR:
            WPRINT_LIB_INFO(("IOTC_SyncResponse internal error: Allocation Error\n"));
            break;
        case IOTCL_SR_PARSING_ERROR:
            WPRINT_LIB_INFO(
                    ("IOTC_SyncResponse internal error: Parsing error. Please check parameters passed to the request.\n"));
            break;
        default:
            WPRINT_LIB_INFO(("WARN: report_sync_error called, but no error returned?\n"));
            break;
    }
}

static void on_iotconnect_status(IotconnectConnectionStatus status, void *data) {
    if (config.status_cb) {
        config.status_cb(status, data);
    }
}

void iotconnect_sdk_disconnect() {
    iotcl_discovery_free_sync_response(sync_response);
    sync_response = NULL;
    iotc_wiced_mqtt_disconnect();
    iotc_wiced_mqtt_deinit();
    WPRINT_LIB_INFO(("SDK Disconnected\n"));
}

wiced_mqtt_msgid_t iotconnect_sdk_send_packet(const char *data) {
    wiced_mqtt_msgid_t ret = iotc_wiced_mqtt_publish((uint8_t *) data, strlen(data));
    if (ret == WICED_SUCCESS) {
        WPRINT_LIB_INFO(("Error: Failed to publish packet!"));
    }
    return ret;
}

wiced_mqtt_msgid_t iotconnect_sdk_send_data_packet(uint8_t *data, size_t len) {
    wiced_mqtt_msgid_t ret = iotc_wiced_mqtt_publish(data, len);
    if (ret == WICED_SUCCESS) {
        WPRINT_LIB_INFO(("Error: Failed to publish packet!"));
    }
    return ret;
}

static void on_message_intercept(IotclEventData data, IotConnectEventType type) {
    switch (type) {
        case ON_FORCE_SYNC:
            iotconnect_sdk_disconnect();
            sync_response = iotc_wiced_discover(config.env, config.cpid, config.duid, config.num_discovery_tires);
            if (!sync_response || sync_response->ds != IOTCL_SR_OK) {
                report_sync_error(sync_response);
                iotcl_discovery_free_sync_response(sync_response);
                sync_response = NULL;
                return;
            }
            (void) iotc_wiced_mqtt_init(&mqtt_config, &config.security);
            break;
        case ON_CLOSE:
            WPRINT_LIB_INFO(("Got a disconnect request. Closing the mqtt connection. Device restart is required.\n"));
            iotconnect_sdk_disconnect();
            break;
        default:
            break; // not handling nay other messages
    }

    if (NULL != config.msg_cb) {
        config.msg_cb(data, type);
    }
}

IotclConfig *iotconnect_sdk_get_lib_config() {
    return iotcl_get_config();
}

IotconnectClientConfig *iotconnect_sdk_init_and_get_config() {
    memset(&config, 0, sizeof(config));
    return &config;
}

bool iotconnect_sdk_is_connected() {
    return iotc_wiced_mqtt_is_connected();
}

void iotc_on_mqtt_data(const uint8_t *data, size_t len, const uint8_t *topic, const uint32_t topic_len) {
    (void) topic; // ignore topic for now
    (void) topic_len;
    char *str = malloc(len + 1);
    memcpy(str, data, len);
    str[len] = 0;
    if (!iotcl_process_event(str)) {
        WPRINT_LIB_INFO(("Error encountered while processing %s\n", str));
    }
    free(str);
}

///////////////////////////////////////////////////////////////////////////////////
// this the Initialization os IoTConnect SDK
wiced_result_t iotconnect_sdk_init() {
    wiced_result_t ret;

    if (0 == config.num_discovery_tires) {
        config.num_discovery_tires = IOTC_SDK_DEFAULT_NUM_DISCOVERY_TRIES;
    }

    iotc_wiced_discovery_init();
    IotclSyncResponse *local_sync_response = iotc_wiced_discover(
            config.env,
            config.cpid,
            config.duid,
            config.num_discovery_tires
    );
    iotc_wiced_discovery_deinit();

    if (!local_sync_response || local_sync_response->ds != IOTCL_SR_OK) {
        report_sync_error(local_sync_response);
        iotcl_discovery_free_sync_response(local_sync_response);
        local_sync_response = NULL;
        return WICED_ERROR;
    }

    WPRINT_LIB_INFO(("CPID: %.*s***\n", 4, local_sync_response->cpid));
    WPRINT_LIB_INFO(("ENV:  %s\n", config.env));

    memset(&mqtt_config, 0, sizeof(mqtt_config));
    mqtt_config.sr = local_sync_response; // FIXME sr is a local pointer that will go out of scope
    mqtt_config.data_cb = iotc_on_mqtt_data;
    mqtt_config.status_cb = on_iotconnect_status;
    mqtt_config.mqtt_timeout_ms = config.mqtt_timeout_ms; // if it is not assigned, the mqtt module will default it

    ret = iotc_wiced_mqtt_init(&mqtt_config, &config.security);
    if (WICED_SUCCESS != ret) {
        return ret;
    }

    lib_config.device.env = config.env;
    lib_config.device.cpid = config.cpid;
    lib_config.device.duid = config.duid;
    lib_config.telemetry.dtg = local_sync_response->dtg; // FIXME sr is a local pointer that will go out of scope
                                                         // unclear what happens to its pointers?
    lib_config.event_functions.ota_cb = config.ota_cb;
    lib_config.event_functions.cmd_cb = config.cmd_cb;

    // intercept internal processing and forward to client
    lib_config.event_functions.msg_cb = on_message_intercept;

    if (!iotcl_init(&lib_config)) {
        WPRINT_LIB_INFO(("Failed to initialize the IoTConnect Lib\n"));
    }

    return 0;
}
