#ifndef _DELIVERY_OPTIMIZATION_DO_CONFIG_H
#define _DELIVERY_OPTIMIZATION_DO_CONFIG_H

/* User Note:
This file exposes apis for helping set an iot_connection_string for the DO agent, so that an application written in c can supply a Microsoft Connected Cache device's hostname
While the apis will compile for all platforms, they only serve a purpose for the DeliveryOptimization Agent on linux devices, all other usage will fail and return e_not_impl
*/

#ifdef __cplusplus
extern "C"
{
#endif

// Keeping this as a C API to enable ADU client to call from its lower layer

// Call this method to let DO know the IoT connection string being used for the device.
// Ideally this will be called early in the IoT application startup phase. For example,
// after ADU client successfully establishes connection with IoT hub.
// Returns: 0 on success, error code otherwise
int deliveryoptimization_set_iot_connection_string(const char* value);

// Future: Version and any future methods could be moved out from the extern C area
// (and use C++ features) if ADU client does not have a need to call these from its lower layer.

// Free the returned char pointer using deliveryoptimization_free_version_buf().
// Return can be NULL if memory allocation failed.
char* deliveryoptimization_get_components_version();

void deliveryoptimization_free_version_buf(char** ppBuffer);

#ifdef __cplusplus
}
#endif

#endif // _DELIVERY_OPTIMIZATION_DO_CONFIG_H
