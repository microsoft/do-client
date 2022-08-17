// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "do_download_status.h"
#include "do_error_helpers.h"

namespace microsoft
{
namespace deliveryoptimization
{

bool download_status::is_error() const noexcept
{
    return _errorCode != 0;
}

bool download_status::is_transient_error() const noexcept
{
    return (_state == download_state::paused) && (_errorCode == 0) && (_extendedErrorCode != 0);
}

bool download_status::is_complete() const noexcept
{
    return _state == download_state::transferred;
}

std::error_code download_status::error_code() const noexcept
{
    return details::make_error_code(_errorCode);
}

std::error_code download_status::extended_error_code() const noexcept
{
    return details::make_error_code(_extendedErrorCode);
}

} // namespace deliveryoptimization
} // namespace microsoft
