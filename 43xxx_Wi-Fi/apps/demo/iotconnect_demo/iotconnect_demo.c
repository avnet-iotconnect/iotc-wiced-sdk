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

/** @file
 *
 */

#include "wiced.h"

#include <time.h>


#include "mqtt_api.h"
#include "sntp.h"

#include "iotconnect_client_config.h"

#include "iotconnect_lib.h"
#include "iotconnect_telemetry.h"
#include "iotc_sdk.h"
#include "resources.h"

#define MQTT_MAX_RESOURCE_SIZE              (4000)
#define MAIN_APP_VERSION                    "00.00.01"

//static wiced_worker_thread_t mqtt_send_thread;

static wiced_result_t get_credentials_from_resources(wiced_mqtt_security_t *s);

/*
 * time() function implementation, required for IotConnect C Library
 */
time_t time(time_t *timer) {
    uint64_t time_ms;
    wiced_time_get_utc_time_ms((wiced_utc_time_ms_t * ) & time_ms);
    return time_ms / 1000;
}

#if 0
static wiced_result_t mqtt_send_handler( void* arg ) {
    const char* msg = ( const char*) arg;
    WPRINT_APP_INFO(("out: %s\n", msg));
    IotConnectSdk_SendPacket(msg);
    IOTCL_DestroySerialized(msg);
    return WICED_SUCCESS;
}
#endif

static void on_command(IOTCL_EVENT_DATA data) {
    const char *command = IOTCL_CloneCommand(data);
    if (NULL != command) {
        WPRINT_APP_INFO(("Received command: %s\n", command));
        free((void *) command);
    }
    const char *ack = IOTCL_CreateAckStringAndDestroyEvent(data, false, "Not implemented");
    if (NULL != ack) {
        WPRINT_APP_INFO(("Sent CMD ack: %s\n", ack));
        IotConnectSdk_SendPacket(ack);
        //wiced_rtos_send_asynchronous_event( &mqtt_send_thread, mqtt_send_handler, (void*)ack );
        free((void *) ack);
    } else {
        WPRINT_APP_ERROR(("Error while creating the ack JSON\n"));
    }
}

static void on_connection_status(IOT_CONNECT_STATUS status) {
    // Add your own status handling
    switch (status) {
        case MQTT_CONNECTED:
            WPRINT_APP_INFO(("IoTConnect MQTT Connected\n"));
            break;
        case MQTT_DISCONNECTED:
            WPRINT_APP_INFO(("IoTConnect MQTT Disonnected\n"));
            break;
        case MQTT_FAILED:
        default:
            WPRINT_APP_ERROR(("IoTConnect MQTT ERROR\n"));
            break;
    }
}

static void on_ota(IOTCL_EVENT_DATA data) {
    const char *message = NULL;
    char *url = IOTCL_CloneDownloadUrl(data, 0);
    bool success = false;
    if (NULL != url) {
        const char *version = IOTCL_CloneSwVersion(data);
        WPRINT_APP_ERROR(("Received OTA Download URL: %s for version %s \n", url, version));
        message = "OTA not supported";
        free((void *) url);
        free((void *) version);
    } else {
        // compatibility with older events
        // This app does not support FOTA with older back ends, but the user can add the functionality
        const char *command = IOTCL_CloneCommand(data);
        if (NULL != command) {
            // URL will be inside the command
            WPRINT_APP_INFO(("Command is: %s\n", command));
            message = "Back end version 1.0 not supported by the app";
            free((void *) command);
        }
    }
    const char *ack = IOTCL_CreateAckStringAndDestroyEvent(data, success, message);
    if (NULL != ack) {
        WPRINT_APP_INFO(("Sent OTA ack: %s\n", ack));
        //wiced_rtos_send_asynchronous_event( &mqtt_send_thread, mqtt_send_handler, (void*)ack );
        IotConnectSdk_SendPacket(ack);
        free((void *) ack);
    }
}

static void publish_telemetry() {
    IOTCL_MESSAGE_HANDLE msg = IOTCL_TelemetryCreate(IotConnectSdk_GetLibConfig());
    wiced_rtos_delay_milliseconds(1000);

    IOTCL_TelemetrySetString(msg, "version", MAIN_APP_VERSION);

    IOTCL_TelemetrySetNumber(msg, "cpu", 33);

    const char *str = IOTCL_CreateSerializedString(msg, false);
    IOTCL_TelemetryDestroy(msg);
    WPRINT_APP_INFO(("Sending: %s\n", str));
    //wiced_rtos_send_asynchronous_event( &mqtt_send_thread, mqtt_send_handler, (void*)str );
    IotConnectSdk_SendPacket(str);
    IOTCL_DestroySerialized(str);
}

void application_start(void) {
    wiced_result_t ret = WICED_SUCCESS;

    ret = wiced_init();

#if 0
    ret = wiced_rtos_create_worker_thread( &mqtt_send_thread, WICED_DEFAULT_LIBRARY_PRIORITY, 4096, 4 );
    if (ret != WICED_SUCCESS) {
        WPRINT_APP_ERROR(("Error while trying to create a worker thread\n"));
        return;
    }
#endif

    /* Bringup the network interface */
    wiced_network_up(WICED_STA_INTERFACE, WICED_USE_EXTERNAL_DHCP_SERVER, NULL);


    IOTCONNECT_CLIENT_CONFIG *config = IotConnectSdk_InitAndGetConfig();

    ret = get_credentials_from_resources(&config->security);

    if (WICED_SUCCESS != ret) {
        WPRINT_APP_ERROR(("Failed to get MQTT credentials from resources\n"));
        return;
    }

    /* IoTConnect requires timestamp.
     * Enable automatic time synchronisation and configure to synchronise once a day.
     */
    WPRINT_APP_INFO(("Fetching time from the SNTP server. Waiting... \n"));
    sntp_start_auto_time_sync(1 * DAYS);
    WPRINT_APP_INFO(("Time obtained.\n"));

    for (int i = 0; i < 1; i++) {
        uint64_t time_ms;
        wiced_time_get_utc_time_ms((wiced_utc_time_ms_t * ) & time_ms);
        WPRINT_APP_INFO(("WICED time now. %llu\n", time_ms));
        WPRINT_APP_INFO(("Time now. %llu\n", time(NULL)));
    }

    config->cpid = IOTCONNECT_CPID;
    config->duid = IOTCONNECT_DUID;
    config->env = IOTCONNECT_ENV;

    config->cmd_cb = on_command;
    config->ota_cb = on_ota;
    config->status_cb = on_connection_status;

    ret = IotConnectSdk_Init();
    if (WICED_SUCCESS != ret) {
        WPRINT_APP_ERROR(("Failed to initialize the SDK\n"));
        return;
    }

    int i = 0;
    while (ret == IotConnectSdk_IsConnected() && i < 10) {

        publish_telemetry();

        wiced_rtos_delay_milliseconds(20000);
        i++;

    }
    IotConnectSdk_Disconnect();
    //wiced_rtos_delete_worker_thread( &mqtt_send_thread );

    /* Free security resources, only needed at initialization */
    resource_free_readonly_buffer(&resources_apps_DIR_iotconnect_demo_DIR_rootca_cer, config->security.ca_cert);
    resource_free_readonly_buffer(&resources_apps_DIR_iotconnect_demo_DIR_client_cer, config->security.cert);
    resource_free_readonly_buffer(&resources_apps_DIR_iotconnect_demo_DIR_privkey_cer, config->security.key);


}

static wiced_result_t get_credentials_from_resources(wiced_mqtt_security_t *s) {
    uint32_t size_out = 0;
    wiced_result_t result = WICED_ERROR;


    if (s->ca_cert) {
        WPRINT_APP_ERROR(
                ("Security Credentials already set(not NULL). Abort Reading from Resources...\n"));
        return WICED_SUCCESS;
    }

    /* Get Root CA certificate filename: 'rootca.cer' located @ resources/apps/iotconnect_demo folder */
    result = resource_get_readonly_buffer(&resources_apps_DIR_iotconnect_demo_DIR_rootca_cer, 0, MQTT_MAX_RESOURCE_SIZE,
                                          &size_out, (const void **) &s->ca_cert);
    if (result != WICED_SUCCESS) {
        goto _fail_certificate_load;
    }
    if (size_out < 64) {
        WPRINT_APP_ERROR(
                ("Invalid Root CA Certificate! Replace the dummy certificate with the correct cert in your application resources.\n"));
        resource_free_readonly_buffer(&resources_apps_DIR_iotconnect_demo_DIR_rootca_cer, (const void *) s->ca_cert);
        goto _fail_certificate_load;
    }

    s->ca_cert_len = size_out;

    /* Get Publisher's Certificate filename: 'client.cer' located @ resources/apps/iotconnect_demo  folder */
    result = resource_get_readonly_buffer(&resources_apps_DIR_iotconnect_demo_DIR_client_cer, 0, MQTT_MAX_RESOURCE_SIZE,
                                          &size_out, (const void **) &s->cert);
    if (result != WICED_SUCCESS) {
        goto _fail_client_certificate;
    }
    if (size_out < 64) {
        WPRINT_APP_ERROR(
                ("Invalid Device Certificate! Replace the dummy certificate with the correct cert in your application resources.\n"));
        resource_free_readonly_buffer(&resources_apps_DIR_iotconnect_demo_DIR_client_cer, (const void *) s->cert);
        goto _fail_client_certificate;
    }

    s->cert_len = size_out;

    /* Get Publisher's Private Key filename: 'privkey.cer' located @ resources/apps/iotconnect_demo folder */
    result = resource_get_readonly_buffer(&resources_apps_DIR_iotconnect_demo_DIR_privkey_cer, 0,
                                          MQTT_MAX_RESOURCE_SIZE, &size_out, (const void **) &s->key);
    if (result != WICED_SUCCESS) {
        goto _fail_private_key;
    }
    if (size_out < 64) {
        WPRINT_APP_ERROR(
                ("Invalid Device Private-Key! Replace the dummy Private-key with the correct key in your application resources.\n"));
        resource_free_readonly_buffer(&resources_apps_DIR_iotconnect_demo_DIR_privkey_cer, (const void *) s->key);
        goto _fail_private_key;
    }
    s->key_len = size_out;

    return WICED_SUCCESS;

    _fail_private_key:
    resource_free_readonly_buffer(&resources_apps_DIR_iotconnect_demo_DIR_client_cer, (const void *) s->cert);
    _fail_client_certificate:
    resource_free_readonly_buffer(&resources_apps_DIR_iotconnect_demo_DIR_rootca_cer, (const void *) s->ca_cert);
    _fail_certificate_load:
    return WICED_ERROR;
}
