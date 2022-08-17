// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _DELIVERY_OPTIMIZATION_DO_ERRORS_H
#define _DELIVERY_OPTIMIZATION_DO_ERRORS_H

#if (DO_ENABLE_EXCEPTIONS)
#include <exception>
#endif
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

#if (DO_ENABLE_EXCEPTIONS)
class exception : public std::exception
{
public:
    exception(std::error_code code);
    exception(int32_t code);
    exception(errc code);

    const char* what() const noexcept override;

    const std::error_code& error_code() const;

private:
    std::error_code _code;
    std::string _msg;
};

#endif // DO_ENABLE_EXCEPTIONS

} //namespace deliveryoptimization
} //namespace microsoft

#endif //_DELIVERY_OPTIMIZATION_DO_ERRORS_H
