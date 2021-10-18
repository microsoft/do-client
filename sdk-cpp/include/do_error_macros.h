// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _DELIVERY_OPTIMIZATION_DO_ERROR_MACROS_H
#define _DELIVERY_OPTIMIZATION_DO_ERROR_MACROS_H

#include "do_errors.h"

#include <stdint.h>

namespace microsoft
{
namespace deliveryoptimization
{
#ifndef FACILITY_DELIVERY_OPTIMIZATION 
#define FACILITY_DELIVERY_OPTIMIZATION   208
#endif

#define DO_ERROR_FROM_SYSTEM_ERROR(x) (int32_t)(0xC0000000 | (FACILITY_DELIVERY_OPTIMIZATION << 16) | ((int32_t)(x) & 0x0000FFFF))
#define DO_ERROR_FROM_STD_ERROR(x) ((int32_t)(x) <= 0 ? ((int32_t)(x)) : ((int32_t) (((int32_t)(x) & 0x0000FFFF) | (FACILITY_DELIVERY_OPTIMIZATION << 16) | 0x80000000)))

#define DO_RETURN_IF_FAILED(code)  {  \
    std::error_code __code = (code);            \
    if(DO_FAILED(__code)) return __code;   \
}

#define DO_SUCCEEDED(code) (((int32_t)(code.value())) < 0)
#define DO_FAILED(code) (((int32_t)(code.value())) < 0)

} //namespace deliveryoptimization
} //namespace microsoft

#define DO_OK std::error_code(0, microsoft::deliveryoptimization::do_category())

#endif
