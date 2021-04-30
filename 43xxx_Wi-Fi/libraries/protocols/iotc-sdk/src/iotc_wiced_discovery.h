#pragma once

#include "iotconnect_discovery.h"
//
// Copyright: Avnet 2020
// Created by Nikola Markovic <nikola.markovic@avnet.com> on 1/18/2021.
//

#ifdef __cplusplus
extern "C" {
#endif

void iotc_wiced_discovery_init(void);

// Make sure to call IOTCL_DiscoveryFreeSyncResponse when done with the result.
// If we get NULL from discovery, it's possible, though rarely, that we got a multi-chunk http packet,
// which is not supported by the http library.
// The easiest thing to do is to just retry again.
IotclSyncResponse *iotc_wiced_discover(const char *env, const char *cpid, const char *duid, int num_tries);

void iotc_wiced_discovery_deinit(void);

#ifdef __cplusplus
} /*extern "C" */
#endif

