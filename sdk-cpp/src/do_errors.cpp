// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "do_errors.h"
#include "do_error_helpers.h"

namespace microsoft
{
namespace deliveryoptimization
{

#if defined(DO_ENABLE_EXCEPTIONS)

exception::exception(std::error_code code) :
    _code(std::move(code)),
    _msg(code.message())
{
}

exception::exception(int32_t code) :
    exception(std::error_code(code, details::do_category()))
{
}

exception::exception(errc code) :
    exception(std::error_code(static_cast<int32_t>(code), details::do_category()))
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

#endif // DO_ENABLE_EXCEPTIONS

} // namespace deliveryoptimization
} // namespace microsoft
