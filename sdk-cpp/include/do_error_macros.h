// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _DELIVERY_OPTIMIZATION_DO_ERROR_MACROS_H
#define _DELIVERY_OPTIMIZATION_DO_ERROR_MACROS_H

/*
TODO(jimson): Move these error macros into a shared header for this project
These error macros are defined behind guards so that they do not redefine the macro for for platforms/toolchains in which they already exist
When moving into a shared file, definitions for some macros in doclient are behind the docli namespace, and they also log errors - sdk has no notion of logging, so logging macros should be no-ops 
*/

#include <stdint.h>

namespace microsoft
{
namespace deliveryoptimization
{

#ifndef HRESULT
using HRESULT = int32_t;
#endif

#ifndef S_OK
#define S_OK            ((HRESULT)0L)
#endif

#ifndef RETURN_IF_FAILED
#define RETURN_IF_FAILED(hr)  {  \
    int32_t __hr = (hr);            \
    if(FAILED(__hr)) return __hr;   \
}
#endif

#ifndef FACILITY_DELIVERY_OPTIMIZATION 
#define FACILITY_DELIVERY_OPTIMIZATION   208
#endif

//TODO(jimson) Looks like these conversions are causing test issues with tests
#define DO_ERROR_FROM_SYSTEM_ERROR(x) (int32_t)(0xC0000000 | (FACILITY_DELIVERY_OPTIMIZATION << 16) | ((int32_t)(x) & 0x0000FFFF))
#define DO_ERROR_FROM_STD_ERROR(x) ((int32_t)(x) <= 0 ? ((int32_t)(x)) : ((int32_t) (((int32_t)(x) & 0x0000FFFF) | (FACILITY_DELIVERY_OPTIMIZATION << 16) | 0x80000000)))

#ifndef FAILED
#define FAILED(hr) (((int32_t)(hr)) < 0)
#endif

#ifndef SUCCEEDED
#define SUCCEEDED(hr) (((int32_t)(hr)) >= 0)
#endif

} //namespace deliveryoptimization
} //namespace microsoft

#endif
