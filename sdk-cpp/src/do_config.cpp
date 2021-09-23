#include "do_config.h"

#include "do_config_internal.h"

extern "C" int deliveryoptimization_set_iot_connection_string(const char* value)
{
    return internal_set_iot_connection_string(value);
}

extern "C" char* deliveryoptimization_get_components_version()
{
    return internal_get_components_version();
}

extern "C" void deliveryoptimization_free_version_buf(char** ppBuffer)
{
    internal_free_version_buf(ppBuffer);
}

