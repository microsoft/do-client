#include "do_exceptions_internal.h"

namespace microsoft::deliveryoptimization::details
{

void ThrowException(int32_t errorCode)
{
    throw exception(errorCode);
}

void ThrowException(std::errc error)
{
    ThrowException(std::make_error_code(error));
}

void ThrowException(std::error_code error)
{
    throw exception(error);
}

void ThrowException(errc errorCode)
{
    throw exception(errorCode);
}

} // namespace microsoft::deliveryoptimization::details
