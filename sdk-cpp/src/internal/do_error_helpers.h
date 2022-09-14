// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _DELIVERY_OPTIMIZATION_DO_ERROR_HELPERS_H
#define _DELIVERY_OPTIMIZATION_DO_ERROR_HELPERS_H

#include <cstdint>
#include <exception>
#include <system_error>
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

#ifdef DO_ENABLE_EXCEPTIONS

class exception : public std::exception
{
public:
    exception(std::error_code code) :
        _code(code),
        _msg(code.message())
    {
    }

    exception(int32_t code) :
        exception(std::error_code(code, do_category()))
    {
    }

    exception(errc code) :
        exception(std::error_code(static_cast<int32_t>(code), do_category()))
    {
    }

    const char* what() const noexcept override
    {
        return _msg.c_str();
    }

    const std::error_code& error_code() const
    {
        return _code;
    }

private:
    std::error_code _code;

    // std::error_code::message() has a by-value return. Store it here for implementation of what().
    std::string _msg;
};

inline void throw_if_fail(std::error_code errorCode)
{
    if (errorCode)
    {
        throw exception(errorCode);
    }
}

inline void ThrowException(std::error_code errorCode)
{
    throw exception(errorCode);
}

inline void ThrowException(std::errc errorCode)
{
    ThrowException(std::make_error_code(errorCode));
}

inline void ThrowException(int32_t errorCode)
{
    throw exception(errorCode);
}

inline void ThrowException(errc errorCode)
{
    throw exception(errorCode);
}

#endif  // DO_ENABLE_EXCEPTIONS

} // namespace details
} //namespace deliveryoptimization
} //namespace microsoft

#endif // _DELIVERY_OPTIMIZATION_DO_ERROR_HELPERS_H
