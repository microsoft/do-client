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

const std::error_category& error_code::category() const noexcept
{
    return do_category();
}

} // namespace deliveryoptimization
} // namespace microsoft

#endif //DO_ENABLE_EXCEPTIONS
