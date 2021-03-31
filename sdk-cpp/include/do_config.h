#pragma once

#if (DO_PLATFORM_ID == DO_PLATFORM_ID_LINUX)
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

#endif // (DO_PLATFORM_ID == DO_PLATFORM_ID_LINUX)