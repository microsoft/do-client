
#include "do_config_internal.h"

#include "do_errors.h"

namespace msdo = microsoft::deliveryoptimization;

char* internal_get_components_version()
{
    return nullptr;
}

void internal_free_version_buf(char** ppBuffer)
{
    if (*ppBuffer)
    {
        free(*ppBuffer);
        *ppBuffer = NULL;
    }
}
