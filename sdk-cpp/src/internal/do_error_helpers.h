// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _DELIVERY_OPTIMIZATION_DO_ERROR_HELPERS_H
#define _DELIVERY_OPTIMIZATION_DO_ERROR_HELPERS_H

#include <stdint.h>
#include "do_errors.h"

namespace microsoft
{
namespace deliveryoptimization
{
#ifndef FACILITY_DELIVERY_OPTIMIZATION
#define FACILITY_DELIVERY_OPTIMIZATION   208
#endif

#define DO_ERROR_FROM_STD_ERROR(x) ((int32_t)(x) <= 0 ? ((int32_t)(x)) : ((int32_t) (((int32_t)(x) & 0x0000FFFF) | (FACILITY_DELIVERY_OPTIMIZATION << 16) | 0x80000000)))

#define DO_RETURN_IF_FAILED(errorCode)  {  \
    std::error_code __code = (errorCode);            \
    if (__code) return __code;   \
}

#if (DO_ENABLE_EXCEPTIONS)
inline void throw_if_fail(std::error_code errorCode)
{
    if (errorCode)
    {
        throw exception(errorCode);
    }
}
#endif

} //namespace deliveryoptimization
} //namespace microsoft

#define DO_OK std::error_code(0, microsoft::deliveryoptimization::do_category())

#endif // _DELIVERY_OPTIMIZATION_DO_ERROR_HELPERS_H
