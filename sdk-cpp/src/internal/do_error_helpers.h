// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _DELIVERY_OPTIMIZATION_DO_ERROR_HELPERS_H
#define _DELIVERY_OPTIMIZATION_DO_ERROR_HELPERS_H

#include <cstdint>
#include "do_errors.h"

namespace microsoft
{
namespace deliveryoptimization
{
namespace details
{

#ifndef FACILITY_DELIVERY_OPTIMIZATION
#define FACILITY_DELIVERY_OPTIMIZATION   208
#endif

#define DO_RETURN_IF_FAILED(errorCode)  {   \
    std::error_code __code = (errorCode);   \
    if (__code) return __code; }

// Error code value for success/no-error case
#define DO_OK std::error_code()

// Category error type for Delivery Optimization errors
class error_category : public std::error_category
{
public:
    const char* name() const noexcept override;

    std::string message(int32_t code) const override;
};

const error_category& do_category();

inline std::error_code make_error_code(std::errc e)
{
    return std::make_error_code(e);
}

inline std::error_code make_error_code(int32_t e)
{
    return std::error_code(e, do_category());
}

inline std::error_code make_error_code(errc e)
{
    return std::error_code(static_cast<int32_t>(e), do_category());
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

} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft

#endif // _DELIVERY_OPTIMIZATION_DO_ERROR_HELPERS_H
