// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _DELIVERY_OPTIMIZATION_DO_ERRORS_H
#define _DELIVERY_OPTIMIZATION_DO_ERRORS_H

#include <cstdint>
#include <system_error>

namespace microsoft
{
namespace deliveryoptimization
{

enum class errc : int32_t
{
    e_not_impl                  = static_cast<int32_t>(0x80004001), // E_NOTIMPL
    unexpected                  = static_cast<int32_t>(0x8000FFFF), // E_UNEXPECTED
    invalid_arg                 = static_cast<int32_t>(0x80070057), // E_INVALIDARG
    not_found                   = static_cast<int32_t>(0x80070490), // E_NOT_SET (ERROR_NOT_FOUND)
    no_service                  = static_cast<int32_t>(0x80D01001), // DO_E_NO_SERVICE
    download_no_progress        = static_cast<int32_t>(0x80D02002), // DO_E_DOWNLOAD_NO_PROGRESS
    do_e_unknown_property_id    = static_cast<int32_t>(0x80D02011), // DO_E_UNKNOWN_PROPERTY_ID
    do_e_invalid_state          = static_cast<int32_t>(0x80D02013), // DO_E_INVALID_STATE
};

} //namespace deliveryoptimization
} //namespace microsoft

#endif //_DELIVERY_OPTIMIZATION_DO_ERRORS_H
