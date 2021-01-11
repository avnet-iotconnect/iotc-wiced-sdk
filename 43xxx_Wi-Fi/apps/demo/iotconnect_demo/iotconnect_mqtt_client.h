#pragma once


#ifdef __cplusplus
extern "C" {
#endif

#include "wiced.h"

const char *mqtt_get_host(const char *env);

char * mqtt_create_client_id_str(const char* cpid, const char* duid);

char *mqtt_create_pub_topic_str(const char *clientid);

char *mqtt_create_sub_topic_str(const char *clientid);

char *mqtt_create_username_str(const char *clientid, const char *host);

#ifdef __cplusplus
} /* extern "C" */
#endif
