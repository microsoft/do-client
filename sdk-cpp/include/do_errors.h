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
    e_not_impl                  = -2063400958,
    unexpected                  = -2147418113,
    invalid_arg                 = -2147024809,
    not_found                   = -2147023728,
    no_service                  = -2133848063,
    download_no_progress        = -2133843966,
    do_e_invalid_state          = -2133843949, // TODO: Revisit convention here - should separate error code enum be used for do_e* errors?
    do_e_unknown_property_id    = -2133843951
};

} //namespace deliveryoptimization
} //namespace microsoft

#endif //_DELIVERY_OPTIMIZATION_DO_ERRORS_H
