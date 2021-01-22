//
// Copyright: Avnet 2020
// Created by Nikola Markovic <nikola.markovic@avnet.com> on 1/18/2021.
//

#include "iotc_wiced_discovery.h"
#include "cert/iotconnect_api_certs.h"

#include <stdlib.h>
#include <wiced.h>
#include "wiced_tls.h"
#include "http_client.h"

#define PORT 443
#define DNS_TIMEOUT_MS     10000
#define CONNECT_TIMEOUT_MS 3000
#define RECEIVE_BUFFER_MAX_SIZE 1024

static const char *root_ca_certificate = CERT_GODADDY_INT_SECURE_G2;

static char data_buff[RECEIVE_BUFFER_MAX_SIZE];
static http_client_t client;
static http_request_t request;
static wiced_semaphore_t semaphore;

// forward declarations -----------
static void synchronous_rest_call(const char *host, const char *path,
                                  const char *post_data);

static void event_handler(http_client_t *client, http_event_t event,
                          http_response_t *response);

static void set_header_field(http_header_field_t *field, const char *name,
                             const char *value);

static void print_data(char *data, uint32_t length);

static void print_content(char *data, uint32_t length);

void iotc_wiced_discovery_init(void) {
    wiced_result_t result;
    result = wiced_tls_init_root_ca_certificates(root_ca_certificate,
                                                 strlen(root_ca_certificate));
    if (result != WICED_SUCCESS) {
        WPRINT_LIB_INFO(
                ("Error: Root CA certificate failed to initialize: %u\n", result));
        return;
    }

    result = http_client_init(&client, WICED_STA_INTERFACE, event_handler,
                              NULL);
    if (result != WICED_SUCCESS) {
        WPRINT_LIB_INFO(
                ("Error: Failed to initialize HTTP client: %u\n", result));
        return;
    }
    wiced_rtos_init_semaphore(&semaphore);
}

void iotc_wiced_discovery_deinit(void) {
    wiced_rtos_deinit_semaphore(&semaphore);
    http_client_deinit(&client);
}

IOTCL_SyncResponse *iotc_wiced_discover(const char *env, const char *cpid, const char *duid, int num_tries) {
    IOTCL_SyncResponse *sr = NULL;
    for (int tries = num_tries; !sr && (tries > 0); tries--) {
        // we will get data into the data buff, but for now, re-use the data buffer for the URL path
        sprintf(data_buff, "/api/sdk/cpid/%s/lang/M_C/ver/2.0/env/%s", cpid, env);
        synchronous_rest_call(IOTCONNECT_DISCOVERY_HOSTNAME, data_buff, NULL);
        if (strlen(data_buff) > 0) {
            char *json_start = strstr(data_buff, "{");
            IOTCL_DiscoveryResponse *dr = IOTCL_DiscoveryParseDiscoveryResponse(
                    json_start);
            WPRINT_LIB_INFO(("Agent URL: %s\n", dr->url));
            char post_data[IOTCONNECT_DISCOVERY_PROTOCOL_POST_DATA_MAX_LEN + 1] = {
                    0};
            snprintf(post_data,
                     IOTCONNECT_DISCOVERY_PROTOCOL_POST_DATA_MAX_LEN, /*total length should not exceed MTU size*/
                     IOTCONNECT_DISCOVERY_PROTOCOL_POST_DATA_TEMPLATE, cpid, duid);
            char sync_path[strlen(dr->path) + strlen("sync?") + 1];
            sprintf(sync_path, "%ssync?", dr->path);
            synchronous_rest_call(dr->host, sync_path, post_data);
            IOTCL_DiscoveryFreeDiscoveryResponse(dr);
            if (strlen(data_buff) > 0) {
                // char * json_start = strstr(data_buff, "{");
                sr = IOTCL_DiscoveryParseSyncResponse(
                        data_buff);
                if (NULL == sr) {
                    WPRINT_LIB_INFO(("Error: Error encountered while parsing Sync Response\n"));
                } else if (sr->ds == IOTCL_SR_PARSING_ERROR) {
                    if (tries != 1) {
                        sr = NULL;
                        WPRINT_LIB_INFO(
                                ("Error: Parsing error encountered due to lack of chunked HTTP transfer support, trying again...\n"));
                    }
                    // else we have to give up
                }
            }
        }
    }
    return sr;
}

void synchronous_rest_call(const char *host, const char *path,
                           const char *post_data) {
    wiced_ip_address_t ip_address;
    wiced_result_t result;
    http_client_configuration_info_t client_configuration;
    http_header_field_t header[4];

    WPRINT_LIB_INFO(("Resolving IP address of %s\n", host));
    wiced_hostname_lookup(host, &ip_address, DNS_TIMEOUT_MS,
                          WICED_STA_INTERFACE);
    WPRINT_LIB_INFO(
            ("%s is at %u.%u.%u.%u\n", host, (uint8_t)(GET_IPV4_ADDRESS(ip_address) >> 24), (uint8_t)(
                    GET_IPV4_ADDRESS(ip_address) >> 16), (uint8_t)(GET_IPV4_ADDRESS(ip_address) >> 8), (uint8_t)(
                    GET_IPV4_ADDRESS(ip_address) >> 0)));

    WPRINT_LIB_INFO(("URL path is %s\n", path));
    /* Initialize the root CA certificate */
    WPRINT_LIB_INFO(("Connecting to %s\n", host));

    /* configure HTTP client parameters */
    client_configuration.flag =
            (http_client_configuration_flags_t)(HTTP_CLIENT_CONFIG_FLAG_SERVER_NAME
                                                | HTTP_CLIENT_CONFIG_FLAG_MAX_FRAGMENT_LEN);
    client_configuration.server_name = (uint8_t *) host;
    client_configuration.max_fragment_length = TLS_FRAGMENT_LENGTH_4096;
    http_client_configure(&client, &client_configuration);

    /* if you set hostname, library will make sure subject name in the server certificate is matching with host name you are trying to connect. pass NULL if you don't want to enable this check */
    client.peer_cn = (uint8_t *) host;

    if ((result = http_client_connect(&client,
                                      (const wiced_ip_address_t *) &ip_address, PORT, HTTP_USE_TLS,
                                      CONNECT_TIMEOUT_MS)) != WICED_SUCCESS) {
        WPRINT_LIB_INFO(("Error: failed to connect to server: %u\n", result));
        data_buff[0] = 0; // clear the data buffer
        return;
    }

    WPRINT_LIB_INFO(("Connected\n"));

    if (post_data) {
        http_request_init(&request, &client, HTTP_POST, path, HTTP_1_1);
    } else {
        http_request_init(&request, &client, HTTP_GET, path, HTTP_1_1);
    }

    set_header_field(&header[0], HTTP_HEADER_HOST, host);
    set_header_field(&header[1], "Connection: ", "close");
    if (post_data) {
        char content_len_buffer[6]; // a few extra byters to prevent Werror/waring about uint16_t size not fitting
        sprintf(content_len_buffer, "%u", (uint16_t) strlen(post_data));
        set_header_field(&header[2], HTTP_HEADER_CONTENT_TYPE,
                         "application/json");
        set_header_field(&header[3], HTTP_HEADER_CONTENT_LENGTH,
                         content_len_buffer);\
        http_request_write_header(&request, &header[0], 4);

    } else {
        http_request_write_header(&request, &header[0], 2);
    }
    http_request_write_end_header(&request);

    if (post_data) {
        http_request_write(&request, (const uint8_t *) post_data,
                           strlen(post_data));
        // Incase of POST, Inserting two CRLF's to indicated end of header
        http_request_write_end_header(&request);
        http_request_write_end_header(&request);
    }

    http_request_flush(&request);

    data_buff[0] = 0; // clear the data buffer

    if (wiced_rtos_get_semaphore(&semaphore, 20000) != WICED_SUCCESS) {
        WPRINT_LIB_INFO(
                ("Error: timed out after 20s waiting for HTTP communication to complete\n"));
    }
    //http_request_deinit(&request);

    WPRINT_LIB_INFO(("Disconnecting.\n"));
    http_client_disconnect(&client);
    WPRINT_LIB_INFO(("Disconnected.\n"));
}

static void event_handler(http_client_t *client, http_event_t event,
                          http_response_t *response) {
    switch (event) {
        case HTTP_DISCONNECTED: {
            WPRINT_LIB_INFO(("HTTP Disconnected\n"));
            wiced_rtos_set_semaphore(&semaphore);
            http_request_deinit(&request);
            break;
        }

        case HTTP_DATA_RECEIVED: {
            if (response->request == &request) {
                /* Response to first request. Simply print the result */
                WPRINT_LIB_INFO(
                        ("\nRecieved response for request. Content received:\n"));

                /* print only HTTP header */
                if (response->response_hdr != NULL) {
                    WPRINT_LIB_INFO(("\n HTTP Header Information for response:\n"));
                    print_content((char *) response->response_hdr,
                                  response->response_hdr_length);
                }

                /* Print payload information that comes as response body */
                WPRINT_LIB_INFO(("Payload Information for response:\n"));
                print_content((char *) response->payload,
                              response->payload_data_length);
                if (response->payload_data_length > RECEIVE_BUFFER_MAX_SIZE) {
                    WPRINT_LIB_INFO(("Receive buffer larger than %d\n", RECEIVE_BUFFER_MAX_SIZE));
                    return;
                }
                memcpy(data_buff, response->payload, response->payload_data_length);
                if (response->remaining_length == 0) {
                    WPRINT_LIB_INFO(("Received total payload data for response\n"));
                }
            }
            break;
        }
        default:
            break;
    }
}

static void print_data(char *data, uint32_t length) {
    uint32_t a;

    for (a = 0; a < length; a++) {
        WPRINT_LIB_INFO(("%c", data[a]));
    }
}

static void set_header_field(http_header_field_t *header_item,
                             const char *name, const char *value) {
    header_item->field = (char *) name;
    header_item->field_length = strlen(name);
    header_item->value = (char *) value;
    header_item->value_length = strlen(value);
}

static void print_content(char *data, uint32_t length) {
    WPRINT_LIB_INFO(("==============================================\n"));
    WPRINT_LIB_INFO(("len=%lu\n", length));
    print_data((char *) data, length);
    WPRINT_LIB_INFO(("\n==============================================\n"));
}

