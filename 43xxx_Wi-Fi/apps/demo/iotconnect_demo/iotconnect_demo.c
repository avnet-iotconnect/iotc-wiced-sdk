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
#include "iotconnect_wiced_discovery.h"
#include "resources.h"


/** @file
 *
 * Send/Receive MQTT Application
 *
 * This application snippet demonstrates how to use
 * the WICED MQTT client library.
 *
 * Features demonstrated
 *  - MQTT Client initiation
 *  - MQTT Client publish (sending / receiving)
 *  - MQTT Client subscribing
 *
 * To demonstrate the app, work through the following steps.
 *  1. Modify the CLIENT_AP_SSID/CLIENT_AP_PASSPHRASE Wi-Fi credentials
 *     in the wifi_config_dct.h header file to match your Wi-Fi access point
 *  2. Modify the MQTT_BROKER_ADDRESS with your MQTT broker( currently using test.mosquitto.org).
 *  3. Plug the WICED eval board into your computer
 *  4. Open a terminal application and connect to the WICED eval board
 *  5. Build and download the application (to the WICED board)
 *
 * Before running the application on the WICED, Make sure your network settings in which Mqtt ports(1883, 8883-secured) are enabled.
 *
 * After the download completes, the terminal displays WICED startup
 * information and then :
 *  - Joins a Wi-Fi network
 *  - Starts an MQTT connection with the server
 *  - Subscribe for a MQTT/WICED/TOPIC topic.
 *  - Publish "Hello WICED" to the same topic
 *  - Received "Hello WICED" from the mqtt deamon.
 *  - Close the MQTT connection
 *
 *SECURITY:
 *  Security in MQTT is done through TLS. This application is testing MQTT over TLS.
 *  We are securely connecting to public broker(test.mosquitto.org)
 *  Root certifcate is of mosquitto.org is in resources/apps/secure_mqtt/secure_mqtt_root_cacert.cer
 *
 *  NOTE : Application uses only root CA certificate.
 *
 *TROUBLESHOOTING
 *   If you are having difficulty connecting the MQTT broker,
 *   Make sure your Broker is configured correctly
 *
 * NOTES:
 *
 *  - Retransmission: WICED MQTT provides two types of retransmission
 *    * Session continue retransmission: When a session is not set as clean, retransmission
 *      of any queued packets from previous session is done once a connect ACK is received.
 *      Session retransmission is part of the MQTT standard.
 *    * In session retransmission: This basically means trying to resend un ACKed packets.
 *      MQTT left this as an application specific and didn't indicate if/when retransmissions
 *      should occur.  WICED MQTT does retransmissions for unACKed packets on the reception
 *      of a PINGRES (ping response) from the deamon. If keep_alive is disabled (not recommended
 *      , in sessions retransmissions will be disabled.
 *
 *  - The MQTT library doesn't protect against user errors, i.e if a user decided to send a
 *    wrong command in a wrong time, the MQTT library won't stop him. The server would signal
 *    an error though.
 *
 *  - Calling MQTT APIs from the event handler call back functions is possible. The only exception
 *    is the wiced_mqtt_deinit which terminates the thread where the call back is being
 *    issued from.
 */
/******************************************************
 *                      Macros
 ******************************************************/
#define RUN_COMMAND_PRINT_STATUS(command, ok_message, error_message)   \
        {                                                                                   \
            ret = (command);                                                                \
            mqtt_print_status( ret, (const char *)ok_message, (const char *)error_message );\
        }


#define RUN_COMMAND_PRINT_STATUS_AND_BREAK_ON_ERROR(command, ok_message, error_message)   \
        {                                                                                   \
            ret = (command);                                                                \
            mqtt_print_status( ret, (const char *)ok_message, (const char *)error_message );\
            if ( ret != WICED_SUCCESS ) break;                                              \
        }


/******************************************************
 *                    Constants
 ******************************************************/

#define WICED_MQTT_TIMEOUT                  (5000)

#define WICED_MQTT_DELAY_IN_MILLISECONDS    (1000)

#define MQTT_MAX_RESOURCE_SIZE              (4000)
/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/
static wiced_result_t get_credentials_from_resources(void);

static wiced_result_t mqtt_connection_event_cb(wiced_mqtt_object_t mqtt_object, wiced_mqtt_event_info_t *event);

static wiced_result_t mqtt_wait_for(wiced_mqtt_event_type_t event, uint32_t timeout);

static wiced_result_t
mqtt_conn_open(const char *client_id, const char *username, wiced_mqtt_object_t mqtt_obj, wiced_ip_address_t *address,
               wiced_interface_t interface, wiced_mqtt_callback_t callback, wiced_mqtt_security_t *security);

static wiced_result_t mqtt_conn_close(wiced_mqtt_object_t mqtt_object);

static wiced_result_t mqtt_app_subscribe(wiced_mqtt_object_t mqtt_obj, char *topic, uint8_t qos);

static wiced_result_t mqtt_app_unsubscribe(wiced_mqtt_object_t mqtt_obj, char *topic);

static wiced_result_t
mqtt_app_publish(wiced_mqtt_object_t mqtt_obj, uint8_t qos, char *topic, uint8_t *data, uint32_t data_len);

static void mqtt_print_status(wiced_result_t restult, const char *ok_message, const char *error_message);

/******************************************************
 *               Variable Definitions
 ******************************************************/
static wiced_ip_address_t broker_address;
static wiced_mqtt_callback_t callbacks = mqtt_connection_event_cb;
static wiced_mqtt_event_type_t expected_event;
static wiced_semaphore_t semaphore;
static wiced_mqtt_security_t security;

/******************************************************
 *               Function Definitions
 ******************************************************/


void application_start(void) {
    static wiced_mqtt_object_t mqtt_object;
    wiced_result_t ret = WICED_SUCCESS;

    wiced_init();

    /* Memory allocated for mqtt object*/
    mqtt_object = (wiced_mqtt_object_t) malloc(WICED_MQTT_OBJECT_MEMORY_SIZE_REQUIREMENT);
    if (mqtt_object == NULL) {
        WPRINT_APP_ERROR(("Don't have memory to allocate for mqtt object...\n"));
        return;
    }

    ret = get_credentials_from_resources();
    if (ret != WICED_SUCCESS) {
        WPRINT_APP_INFO(("[Application/AWS] Error fetching credentials from resources\n"));
        return;
    }

    /* Bringup the network interface */
    wiced_network_up(WICED_STA_INTERFACE, WICED_USE_EXTERNAL_DHCP_SERVER, NULL);

    /* IoTConnect requires timestamp.
     * Enable automatic time synchronisation and configure to synchronise once a day.
     */

    WPRINT_APP_INFO(("Fetching time from the SNTP server. Waiting... \n"));

    sntp_start_auto_time_sync(1 * DAYS);

    WPRINT_APP_INFO(("Time obtained.\n"));

    iotc_wiced_discovery_init();
    IOTCL_SyncResponse* sr = iotc_wiced_discover(IOTCONNECT_ENV, IOTCONNECT_CPID, IOTCONNECT_DUID);
    iotc_wiced_discovery_deinit();

    IOTCL_CONFIG config;
    memset(&config, 0, sizeof(config));
    config.device.cpid = IOTCONNECT_CPID;
    config.device.duid = IOTCONNECT_DUID;
    config.device.env = IOTCONNECT_ENV;
    config.telemetry.dtg = sr->dtg;
    IOTCL_Init(&config);

    WPRINT_APP_INFO(("Resolving IP address for MQTT broker for %s environment: %s \n", IOTCONNECT_ENV, sr->broker.host));
    ret = wiced_hostname_lookup(sr->broker.host, &broker_address, 10000, WICED_STA_INTERFACE);
    WPRINT_APP_INFO(("Resolved Broker IP: %u.%u.%u.%u\n\n", (uint8_t)(GET_IPV4_ADDRESS(broker_address) >> 24),
            (uint8_t)(GET_IPV4_ADDRESS(broker_address) >> 16),
            (uint8_t)(GET_IPV4_ADDRESS(broker_address) >> 8),
            (uint8_t)(GET_IPV4_ADDRESS(broker_address) >> 0)));
    if (ret == WICED_ERROR || broker_address.ip.v4 == 0) {
        WPRINT_APP_INFO(("Error in resolving DNS\n"));
        return;
    }

    ret = wiced_mqtt_init(mqtt_object);
    if (ret != WICED_SUCCESS) {
        WPRINT_APP_INFO(("[Application/AWS] Failed to init mqtt\n"));
        return;
    }

    wiced_rtos_init_semaphore(&semaphore);

    while (ret == WICED_SUCCESS) {

        WPRINT_APP_INFO(("[MQTT] Opening connection..."));
        RUN_COMMAND_PRINT_STATUS_AND_BREAK_ON_ERROR(
                mqtt_conn_open(sr->broker.client_id, sr->broker.user_name, mqtt_object, &broker_address, WICED_STA_INTERFACE, callbacks,
                               &security), NULL, "Did you configure you broker IP address?\n");

        wiced_rtos_delay_milliseconds(WICED_MQTT_DELAY_IN_MILLISECONDS * 10);

        WPRINT_APP_INFO(("[MQTT] Publishing..."));
        IOTCL_MESSAGE_HANDLE msg = IOTCL_TelemetryCreate();
        IOTCL_TelemetrySetString(msg, "version", "00.00.01");
        IOTCL_TelemetrySetNumber(msg, "cpu", 10);

        const char *str = IOTCL_CreateSerializedString(msg, false);

        WPRINT_APP_INFO(("[MQTT] Sending on %s: %s\n", sr->broker.pub_topic, str));
        RUN_COMMAND_PRINT_STATUS(mqtt_app_publish(
                mqtt_object,
                WICED_MQTT_QOS_DELIVER_AT_LEAST_ONCE,
                sr->broker.pub_topic,
                (uint8_t *) str,
                strlen(str)
        ), NULL, NULL);

        IOTCL_TelemetryDestroy(msg);
        IOTCL_DestroySerialized(str);

        WPRINT_APP_INFO(("[MQTT] Subscribing..."));
        RUN_COMMAND_PRINT_STATUS_AND_BREAK_ON_ERROR(
                mqtt_app_subscribe(mqtt_object, sr->broker.sub_topic, WICED_MQTT_QOS_DELIVER_AT_LEAST_ONCE), NULL, NULL);

        WPRINT_APP_INFO(("[MQTT] Waiting some time for ping exchange...\n\n"));

        wiced_rtos_delay_milliseconds(WICED_MQTT_DELAY_IN_MILLISECONDS * 10);

        WPRINT_APP_INFO(("[MQTT] Unsubscribing..."));
        RUN_COMMAND_PRINT_STATUS(mqtt_app_unsubscribe(mqtt_object, sr->broker.sub_topic), NULL, NULL);

        WPRINT_APP_INFO(("[MQTT] Closing connection..."));
        RUN_COMMAND_PRINT_STATUS_AND_BREAK_ON_ERROR(mqtt_conn_close(mqtt_object), NULL, NULL);

        wiced_rtos_delay_milliseconds(WICED_MQTT_DELAY_IN_MILLISECONDS * 2);

    }
    wiced_rtos_deinit_semaphore(&semaphore);
    WPRINT_APP_INFO(("[MQTT] Deinit connection..."));

    ret = wiced_mqtt_deinit(mqtt_object);
    mqtt_print_status(ret, NULL, NULL);
    IOTCL_DiscoveryFreeSyncResponse(sr);

    /* Free security resources, only needed at initialization */
    resource_free_readonly_buffer(&resources_apps_DIR_iotconnect_demo_DIR_rootca_cer, security.ca_cert);
    resource_free_readonly_buffer(&resources_apps_DIR_iotconnect_demo_DIR_client_cer, security.cert);
    resource_free_readonly_buffer(&resources_apps_DIR_iotconnect_demo_DIR_privkey_cer, security.key);
}

/******************************************************
 *               Static Function Definitions
 ******************************************************/

static wiced_result_t get_credentials_from_resources(void) {
    uint32_t size_out = 0;
    wiced_result_t result = WICED_ERROR;

    wiced_mqtt_security_t *s = &security;

    if (s->ca_cert) {
        WPRINT_APP_INFO(
                ("\n[Application/AWS] Security Credentials already set(not NULL). Abort Reading from Resources...\n"));
        return WICED_SUCCESS;
    }

    /* Get AWS Root CA certificate filename: 'rootca.cer' located @ resources/apps/aws/iot folder */
    result = resource_get_readonly_buffer(&resources_apps_DIR_iotconnect_demo_DIR_rootca_cer, 0, MQTT_MAX_RESOURCE_SIZE,
                                          &size_out, (const void **) &s->ca_cert);
    if (result != WICED_SUCCESS) {
        goto _fail_aws_certificate;
    }
    if (size_out < 64) {
        WPRINT_APP_INFO(
                ("\n[Application/AWS] Invalid Root CA Certificate! Replace the dummy certificate with AWS one[<YOUR_WICED_SDK>/resources/app/aws/iot/'rootca.cer']\n\n"));
        resource_free_readonly_buffer(&resources_apps_DIR_iotconnect_demo_DIR_rootca_cer, (const void *) s->ca_cert);
        goto _fail_aws_certificate;
    }

    s->ca_cert_len = size_out;

    /* Get Publisher's Certificate filename: 'client.cer' located @ resources/apps/aws/greengrass/pubisher folder */
    result = resource_get_readonly_buffer(&resources_apps_DIR_iotconnect_demo_DIR_client_cer, 0, MQTT_MAX_RESOURCE_SIZE,
                                          &size_out, (const void **) &s->cert);
    if (result != WICED_SUCCESS) {
        goto _fail_client_certificate;
    }
    if (size_out < 64) {
        WPRINT_APP_INFO(
                ("\n[Application/AWS] Invalid Device Certificate! Replace the dummy certificate with AWS one[<YOUR_WICED_SDK>/resources/app/aws/greengrass/publisher/'client.cer']\n\n"));
        resource_free_readonly_buffer(&resources_apps_DIR_iotconnect_demo_DIR_client_cer, (const void *) s->cert);
        goto _fail_client_certificate;
    }

    s->cert_len = size_out;

    /* Get Publisher's Private Key filename: 'privkey.cer' located @ resources/apps/aws/greengrass/publisher folder */
    result = resource_get_readonly_buffer(&resources_apps_DIR_iotconnect_demo_DIR_privkey_cer, 0,
                                          MQTT_MAX_RESOURCE_SIZE, &size_out, (const void **) &s->key);
    if (result != WICED_SUCCESS) {
        goto _fail_private_key;
    }
    if (size_out < 64) {
        WPRINT_APP_INFO(
                ("\n[Application/AWS] Invalid Device Private-Key! Replace the dummy Private-key with AWS one[<YOUR_WICED_SDK>/resources/app/aws/greengrass/publisher/'privkey.cer'\n\n"));
        resource_free_readonly_buffer(&resources_apps_DIR_iotconnect_demo_DIR_privkey_cer, (const void *) s->key);
        goto _fail_private_key;
    }
    s->key_len = size_out;

    return WICED_SUCCESS;

    _fail_private_key:
    resource_free_readonly_buffer(&resources_apps_DIR_iotconnect_demo_DIR_client_cer, (const void *) s->cert);
    _fail_client_certificate:
    resource_free_readonly_buffer(&resources_apps_DIR_iotconnect_demo_DIR_rootca_cer, (const void *) s->ca_cert);
    _fail_aws_certificate:
    return WICED_ERROR;
}

/*
 * A simple result log function
 */
static void mqtt_print_status(wiced_result_t result, const char *ok_message, const char *error_message) {
    if (result == WICED_SUCCESS) {
        if (ok_message != NULL) {
            WPRINT_APP_INFO(("OK (%s)\n\n", (ok_message)));
        } else {
            WPRINT_APP_INFO(("OK.\n\n"));
        }
    } else {
        if (error_message != NULL) {
            WPRINT_APP_INFO(("ERROR (%s)\n\n", (error_message)));
        } else {
            WPRINT_APP_INFO(("ERROR.\n\n"));
        }
    }
}

/*
 * Call back function to handle connection events.
 */
static wiced_result_t mqtt_connection_event_cb(wiced_mqtt_object_t mqtt_object, wiced_mqtt_event_info_t *event) {
    switch (event->type) {
        case WICED_MQTT_EVENT_TYPE_CONNECT_REQ_STATUS:
            WPRINT_APP_INFO(("[MQTT] Connected..."));
            if (event->data.conn_ack.err_code != WICED_MQTT_CONN_ERR_CODE_NONE) {
                WPRINT_APP_INFO(("Connection Error code: %d\n", event->data.conn_ack.err_code));
            } else {
                WPRINT_APP_INFO(("Successfully connected\n"));
            }
            break;
        case WICED_MQTT_EVENT_TYPE_DISCONNECTED:
            WPRINT_APP_INFO(("[MQTT] Disconnected..."));
            break;
        default:
            break;
    }
    switch (event->type) {
        case WICED_MQTT_EVENT_TYPE_CONNECT_REQ_STATUS:
        case WICED_MQTT_EVENT_TYPE_DISCONNECTED:
        case WICED_MQTT_EVENT_TYPE_PUBLISHED:
        case WICED_MQTT_EVENT_TYPE_SUBSCRIBED:
        case WICED_MQTT_EVENT_TYPE_UNSUBSCRIBED: {
            expected_event = event->type;
            wiced_rtos_set_semaphore(&semaphore);
        }
            break;
        case WICED_MQTT_EVENT_TYPE_PUBLISH_MSG_RECEIVED: {
            wiced_mqtt_topic_msg_t msg = event->data.pub_recvd;
            WPRINT_APP_INFO(
                    ("[MQTT] Received %.*s  for TOPIC : %.*s\n\n", (int) msg.data_len, msg.data, (int) msg.topic_len, msg.topic));
        }
            break;
        default:
            break;
    }
    return WICED_SUCCESS;
}

/*
 * Call back function to handle channel events.
 *
 * For each event:
 *  - The call back will set the expected_event global to the received event.
 *  - The call back will set the event semaphore to run any blocking thread functions waiting on this event
 *  - Some events will also log other global variables required for extra processing.
 *
 * A thread function will probably be waiting for the received event. Once the event is received and the
 * semaphore is set, the thread function will check for the received event and make sure it matches what
 * it is expecting.
 *
 * Note:  This mechanism is not thread safe as we are using a non protected global variable for read/write.
 * However as this snip is a single controlled thread, there is no risc of racing conditions. It is
 * however not recommended for multi-threaded applications.
 */

/*
 * A blocking call to an expected event.
 */
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
 * Open a connection and wait for WICED_MQTT_TIMEOUT period to receive a connection open OK event
 */
wiced_result_t mqtt_conn_open(
        const char *client_id,
        const char *username,
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
    conninfo.keep_alive = 10;
    conninfo.password = (uint8_t *) "";
    conninfo.username = (uint8_t *) username;
    conninfo.peer_cn = (uint8_t *) "*.azure-devices.net";

    ret = wiced_mqtt_connect(mqtt_obj, address, interface, callback, security, &conninfo);
    if (ret != WICED_SUCCESS) {
        return WICED_ERROR;
    }
    if (mqtt_wait_for(WICED_MQTT_EVENT_TYPE_CONNECT_REQ_STATUS, WICED_MQTT_TIMEOUT) != WICED_SUCCESS) {
        return WICED_ERROR;
    }
    return WICED_SUCCESS;
}

/*
 * Close a connection and wait for 5 seconds to receive a connection close OK event
 */
static wiced_result_t mqtt_conn_close(wiced_mqtt_object_t mqtt_obj) {
    if (wiced_mqtt_disconnect(mqtt_obj) != WICED_SUCCESS) {
        return WICED_ERROR;
    }
    if (mqtt_wait_for(WICED_MQTT_EVENT_TYPE_DISCONNECTED, WICED_MQTT_TIMEOUT) != WICED_SUCCESS) {
        return WICED_ERROR;
    }
    return WICED_SUCCESS;
}

/*
 * Subscribe to WICED_TOPIC and wait for 5 seconds to receive an ACM.
 */
static wiced_result_t mqtt_app_subscribe(wiced_mqtt_object_t mqtt_obj, char *topic, uint8_t qos) {
    wiced_mqtt_msgid_t pktid;
    pktid = wiced_mqtt_subscribe(mqtt_obj, topic, qos);
    if (pktid == 0) {
        return WICED_ERROR;
    }
    if (mqtt_wait_for(WICED_MQTT_EVENT_TYPE_SUBSCRIBED, WICED_MQTT_TIMEOUT) != WICED_SUCCESS) {
        return WICED_ERROR;
    }
    return WICED_SUCCESS;
}

/*
 * Unsubscribe from WICED_TOPIC and wait for 10 seconds to receive an ACM.
 */
static wiced_result_t mqtt_app_unsubscribe(wiced_mqtt_object_t mqtt_obj, char *topic) {
    wiced_mqtt_msgid_t pktid;
    pktid = wiced_mqtt_unsubscribe(mqtt_obj, topic);

    if (pktid == 0) {
        return WICED_ERROR;
    }
    if (mqtt_wait_for(WICED_MQTT_EVENT_TYPE_UNSUBSCRIBED, WICED_MQTT_TIMEOUT * 2) != WICED_SUCCESS) {
        return WICED_ERROR;
    }
    return WICED_SUCCESS;
}

/*
 * Publish (send) WICED_MESSAGE_STR to WICED_TOPIC and wait for 5 seconds to receive a PUBCOMP (as it is QoS=2).
 */
static wiced_result_t
mqtt_app_publish(wiced_mqtt_object_t mqtt_obj, uint8_t qos, char *topic, uint8_t *data, uint32_t data_len) {
    wiced_mqtt_msgid_t pktid;
    pktid = wiced_mqtt_publish(mqtt_obj, topic, data, data_len, qos);

    if (pktid == 0) {
        return WICED_ERROR;
    }

    if (mqtt_wait_for(WICED_MQTT_EVENT_TYPE_PUBLISHED, WICED_MQTT_TIMEOUT) != WICED_SUCCESS) {
        return WICED_ERROR;
    }
    return WICED_SUCCESS;
}
