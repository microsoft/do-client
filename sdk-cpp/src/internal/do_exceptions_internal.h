// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#if defined(DO_ENABLE_EXCEPTIONS)

#ifndef _DELIVERY_OPTIMIZATION_DO_EXCEPTIONS_INTERNAL_H
#define _DELIVERY_OPTIMIZATION_DO_EXCEPTIONS_INTERNAL_H

#include <exception>
#include <stdint.h>
#include <system_error>

#include "do_errors.h"

namespace microsoft
{
namespace deliveryoptimization
{
namespace details
{

void ThrowException(int32_t errorCode);
void ThrowException(std::errc error);
void ThrowException(std::error_code error);
void ThrowException(errc error);

} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft
#endif

#endif
