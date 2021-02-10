#include "do_download_status.h"

namespace microsoft::deliveryoptimization
{

bool download_status::is_error() const
{
    return _errorCode != 0;
}

bool download_status::is_transient_error() const
{
    return (_state == download_state::paused) && (_errorCode == 0) && (_extendedErrorCode != 0);
}

bool download_status::is_complete() const
{
    return _state == download_state::transferred;
}

} // namespace microsoft::deliveryoptimization
