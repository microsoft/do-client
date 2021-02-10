
#include <exception>
#include <stdint.h>
#include <system_error>

#include "do_exceptions.h"

namespace microsoft::deliveryoptimization::details
{
    void ThrowException(int32_t errorCode);
    void ThrowException(std::errc error);
    void ThrowException(std::error_code error);
    void ThrowException(errc error);
}