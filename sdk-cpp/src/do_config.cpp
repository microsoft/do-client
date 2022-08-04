// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "do_config.h"
#include "do_config_internal.h"

// TODO(shishirb) Remove API when DU agent is ready to take the change
extern "C" int deliveryoptimization_set_iot_connection_string(const char*)
{
    return 0;
}

extern "C" char* deliveryoptimization_get_components_version()
{
    return internal_get_components_version();
}

extern "C" void deliveryoptimization_free_version_buf(char** ppBuffer)
{
    internal_free_version_buf(ppBuffer);
}

