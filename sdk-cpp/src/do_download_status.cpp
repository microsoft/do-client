// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "do_download_status.h"

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

} // namespace deliveryoptimization
} // namespace microsoft
