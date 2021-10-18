// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "do_errors.h"

namespace microsoft
{
namespace deliveryoptimization
{

const error_category& do_category()
{
    static error_category instance;
    return instance;
}

std::error_code make_error_code(errc e)
{
    return std::error_code(static_cast<int>(e), do_category());
}
std::error_code make_error_code(int32_t e)
{
    return std::error_code(e, do_category());
}

const char* error_category::name() const noexcept
{
    return "delivery optimization error";
}
std::string error_category::message(int32_t code) const
{
    switch (static_cast<errc>(code))
    {
    default:
        return "unrecognized error";
    }
}

#if defined(DO_ENABLE_EXCEPTIONS)

exception::exception(std::error_code code) :
    _code(std::move(code)),
    _msg(code.message())
{
}

exception::exception(int32_t code) :
    exception(std::error_code(code, do_category()))
{
}

exception::exception(errc code) :
    exception(std::error_code(static_cast<int32_t>(code), do_category()))
{
}

const char* exception::what() const noexcept
{
    return _msg.c_str();
}

const std::error_code& exception::error_code() const
{
    return _code;
}

} // namespace deliveryoptimization
} // namespace microsoft

#endif //DO_ENABLE_EXCEPTIONS
