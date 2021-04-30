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
#include <sntp.h>
#include <mqtt_common.h>
#include "iotconnect_client_config.h"

#include "iotconnect_lib.h"
#include "iotconnect_telemetry.h"
#include "iotc_sdk.h"
#include <resources.h>

#define MQTT_MAX_RESOURCE_SIZE              4000
#define MAIN_APP_VERSION                    "00.00.01"

static wiced_result_t get_credentials_from_resources(wiced_mqtt_security_t *s);

/*
 * time() function implementation, required for IotConnect C Library
 */
time_t time(time_t *timer) {
    uint64_t time_ms;
    wiced_time_get_utc_time_ms((wiced_utc_time_ms_t * ) & time_ms);
    return time_ms / 1000;
}

static void on_command(IotclEventData data) {
    const char *command = iotcl_clone_command(data);
    if (NULL != command) {
        WPRINT_APP_INFO(("Received command: %s\n", command));
        free((void *) command);
    }
    const char *ack = iotcl_create_ack_string_and_destroy_event(data, false, "Not implemented");
    if (NULL != ack) {
        iotconnect_sdk_send_packet(ack);
        WPRINT_APP_INFO(("Sent CMD ack: %s\n", ack));
        free((void *) ack);
    } else {
        WPRINT_APP_ERROR(("Error while creating the ack JSON\n"));
    }
}

static void on_ota(IotclEventData data) {
    const char *message = NULL;
    char *url = iotcl_clone_download_url(data, 0);
    bool success = false;
    if (NULL != url) {
        const char *version = iotcl_clone_sw_version(data);
        WPRINT_APP_INFO(("Received OTA Download URL: %s for version %s \n", url, version));
        message = "OTA not supported";
        free((void *) url);
        free((void *) version);
    } else {
        // compatibility with older than 2.0 back end events
        const char *command = iotcl_clone_command(data);
        if (NULL != command) {
            // URL will be the command argument
            WPRINT_APP_INFO(("Command is: %s\n", command));
            message = "OTA not supported";
            free((void *) command);
        }
    }
    const char *ack = iotcl_create_ack_string_and_destroy_event(data, success, message);
    if (NULL != ack) {
        WPRINT_APP_INFO(("Sent OTA ack: %s\n", ack));
        iotconnect_sdk_send_packet(ack);
        free((void *) ack);
    }
}

static void on_connection_status(IotconnectConnectionStatus status, void *data) {
    // Add your own status handling
    switch (status) {
        case MQTT_CONNECTED:
            WPRINT_APP_INFO(("IoTConnect MQTT Connected\n"));
            break;
        case MQTT_DISCONNECTED:
            WPRINT_APP_INFO(("IoTConnect MQTT Disconnected\n"));
            break;
        case MQTT_PUBLISHED: {
            wiced_mqtt_msgid_t *data_ptr = (wiced_mqtt_msgid_t *) data;
            WPRINT_APP_INFO(("Packet ID %u published\n", *data_ptr));
        }
            break;
        case MQTT_FAILED:
        default:
            WPRINT_APP_ERROR(("IoTConnect MQTT ERROR\n"));
            break;
    }
}

static void publish_telemetry() {
    IotclMessageHandle msg = iotcl_telemetry_create(iotconnect_sdk_get_lib_config());

    iotcl_telemetry_set_string(msg, "version", MAIN_APP_VERSION);
    iotcl_telemetry_set_number(msg, "cpu", 33);

    const char *str = iotcl_create_serialized_string(msg, false);
    iotcl_telemetry_destroy(msg);
    wiced_mqtt_msgid_t pktId = iotconnect_sdk_send_packet(str);
    WPRINT_APP_INFO(("Sending packet ID %u: %s\n", pktId, str));
    iotcl_destroy_serialized(str);
}

void application_start(void) {
    wiced_result_t ret = WICED_SUCCESS;

    ret = wiced_init();

    /* Bring up the network interface */
    wiced_network_up(WICED_STA_INTERFACE, WICED_USE_EXTERNAL_DHCP_SERVER, NULL);

#define PREFIX_LEN (sizeof("wiced-") - 1)
    wiced_mac_t mac;
    char duid[PREFIX_LEN + sizeof(mac.octet) + 1 /* null */] = "wiced-test";
    if (wiced_wifi_get_mac_address(&mac) == WICED_SUCCESS) {
        for (int i = 0; i < sizeof(mac.octet); i++) {
            sprintf(&duid[PREFIX_LEN + i * 2], "%02x", mac.octet[i]);
        }
    }
    /* just print 5 characters */
    WPRINT_APP_INFO(("DUID: %.*s...\n", PREFIX_LEN + 5, duid));


    IotconnectClientConfig *config = iotconnect_sdk_init_and_get_config();

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

    config->duid = duid;
    config->cpid = IOTCONNECT_CPID;
    config->env = IOTCONNECT_ENV;

    config->cmd_cb = on_command;
    config->ota_cb = on_ota;
    config->status_cb = on_connection_status;

    ret = iotconnect_sdk_init();
    if (WICED_SUCCESS != ret) {
        WPRINT_APP_ERROR(("Failed to initialize the SDK\n"));
        return;
    }

    int i = 0;
    while (ret == iotconnect_sdk_is_connected() && i < 10) {

        publish_telemetry();

        wiced_rtos_delay_milliseconds(20000);
        i++;

    }
    iotconnect_sdk_disconnect();

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
