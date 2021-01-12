#include <string.h>
#include <stddef.h>

#include "wiced.h"

#include "iotconnect_client_config.h"

#define MQTT_BROKER_ADDRESS_AVNET            "poc-iotconnect-iothub-eu.azure-devices.net"
#define MQTT_TOPIC_REFIX "devices/"
#define MQTT_PUB_SUFFIX  "/messages/events/"
#define MQTT_SUB_SUFFIX  "/messages/devicebound/#"

/* TODO: Need HTTP sync implementation to return the propper information
 * All these functions can be obosleted */

const char *mqtt_get_host(const char *env)
{
    if (!env || env[0] == 0) {
        return NULL;
    }
    if (strcmp("Avnet", env) == 0 || strcmp("avnetpoc", env) == 0) {
        return "poc-iotconnect-iothub-eu.azure-devices.net";
    } else if (strcmp("qa", env) == 0) {
        return "qa-iconnect-core-iothub-cu.azure-devices.net";
    } else if (strcmp("qa", env) == 0) {
        return "qa-iconnect-core-iothub-cu.azure-devices.net";
    } else {
        return NULL;
    }
}

char *mqtt_create_client_id_str(const char* cpid, const char* duid)
{
    if (!cpid || !duid) {
        return NULL;
    }

    char * ret = malloc(strlen(cpid) + strlen(duid) + 1 /* dash */ + 1 /* terminator */);
    if (!ret) {
        return NULL;
    }
    ret[0] = 0;

    strcat(ret, cpid);
    strcat(ret, "-");
    strcat(ret, duid);

    return ret;
}

char *mqtt_create_pub_topic_str(const char *clientid)
{

    if (!clientid || clientid[0] == 0) {
        return NULL;
    }

    char *ret = malloc(strlen(MQTT_TOPIC_REFIX) + strlen(clientid) + strlen(MQTT_PUB_SUFFIX) + 1 /* terminator */);
    if (!ret) {
        return NULL;
    }
    ret[0] = 0;

    strcat(ret, MQTT_TOPIC_REFIX);
    strcat(ret, clientid);
    strcat(ret, MQTT_PUB_SUFFIX);

    return ret;
}

char *mqtt_create_sub_topic_str(const char *clientid)
{

    if (!clientid || clientid[0] == 0) {
        return NULL;
    }

    char *ret = malloc(strlen(MQTT_TOPIC_REFIX) + strlen(clientid) + strlen(MQTT_SUB_SUFFIX) + 1 /* terminator */);
    if (!ret) {
        return NULL;
    }
    ret[0] = 0;

    strcat(ret, MQTT_TOPIC_REFIX);
    strcat(ret, clientid);
    strcat(ret, MQTT_SUB_SUFFIX);

    return ret;
}

char *mqtt_create_username_str(const char *clientid, const char *host)
{
    if (!clientid || clientid[0] == 0 || !host || host[0] == 0) {
        return NULL;
    }
    //MQTT_BROKER_ADDRESS"/"CLIENT_ID"/?"API_VERSION_STRING;
    char *ret = malloc(strlen(host) + strlen(clientid) + strlen(API_VERSION_STRING) + 2 /* separators etc. */ + 1 /* terminator */);
    if (!ret) {
        return NULL;
    }
    ret[0] = 0;

    strcat(ret, host);
    strcat(ret, "/");
    strcat(ret, clientid);
    strcat(ret, "/?");
    strcat(ret, MQTT_SUB_SUFFIX);

    return ret;
}
