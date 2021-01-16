#ifndef IOTCL_WICED_DICOVERY_H
#define IOTCL_WICED_DICOVERY_H

#include "iotconnect_discovery.h"

#ifdef __cplusplus
extern "C" {
#endif

void iotc_wiced_discovery_init(void);

// Make sure to call IOTCL_DiscoveryFreeSyncResponse when done with the result.
IOTCL_SyncResponse* iotc_wiced_discover(const char* env, const char* cpid, const char* duid);

void iotc_wiced_discovery_deinit(void);

#ifdef __cplusplus
} /*extern "C" */
#endif

#endif /* IOTCL_WICED_DICOVERY_H */
