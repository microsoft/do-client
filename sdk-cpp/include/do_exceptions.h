// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _DELIVERY_OPTIMIZATION_DO_EXCEPTIONS_H
#define _DELIVERY_OPTIMIZATION_DO_EXCEPTIONS_H

#if (DO_ENABLE_EXCEPTIONS)
#include <exception>
#endif
#include <stdint.h>
#include <system_error>

#include "do_error_macros.h"

namespace microsoft
{
namespace deliveryoptimization
{

#ifndef FAILED
#define FAILED(res) (((int32_t)(res)) < 0)
#endif

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

class error_category : public std::error_category
{
public:
    const char* name() const noexcept override;

    std::string message(int code) const override;
};

class exception : public std::exception
{
public:
    // TODO(jimson): With the error macro above, std::error_code is always transformed into an int32_t before throwing
    // Another option could have been creating an error code class which accepts std::error_code, int32_t, and errc as constructor args
    // Look into deprecating this interface when we publish a new MajorVersion
    exception(std::error_code code);
    exception(int32_t code);
    exception(errc code);

    const char* what() const noexcept override;

    int32_t error_code() const;

    const std::error_code& get_error_code() const;

private:
    std::error_code _code;
    std::string _msg;
};

inline void throw_if_fail(int32_t hr)
{
    if (FAILED(hr))
    {
        throw exception(hr);
    }
}

#endif // DO_ENABLE_EXCEPTIONS

} //namespace deliveryoptimization
} //namespace microsoft

#endif //_DELIVERY_OPTIMIZATION_DO_EXCEPTIONS_H
