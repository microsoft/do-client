
#include "do_exceptions.h"

namespace microsoft
{
namespace deliveryoptimization
{
const error_category& error_category_instance()
{
    static error_category instance;
    return instance;
}

const char* error_category::name() const noexcept
{
    return "delivery optimization error";
}
std::string error_category::message(int code) const
{
    switch (static_cast<errc>(code))
    {
    default:
        return "unrecognized error";
    }
}

exception::exception(std::error_code code) :
    _code(std::move(code)),
    _msg(code.message())
{
}

exception::exception(int32_t code) :
    exception(std::error_code(code, error_category_instance()))
{
}

exception::exception(errc code) :
    exception(std::error_code(static_cast<int32_t>(code), error_category_instance()))
{
}

const char* exception::what() const noexcept
{
    return _msg.c_str();
}

int32_t exception::error_code() const
{
    return _code.value();
}

const std::error_code& exception::get_error_code() const
{
    return _code;
}
}
} // namespace microsoft::deliveryoptimization
