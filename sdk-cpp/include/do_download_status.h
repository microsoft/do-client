#ifndef _DELIVERY_OPTIMIZATION_DO_DOWNLOAD_STATUS_H
#define _DELIVERY_OPTIMIZATION_DO_DOWNLOAD_STATUS_H

#include <stdint.h>

namespace microsoft
{
namespace deliveryoptimization
{

enum class download_state
{
    created,
    transferring,
    transferred,
    finalized,
    aborted,
    paused,
};

class download_status
{
public:
    download_status() = default;
    
    download_status(uint64_t bytesTotal, uint64_t bytesTransferred, int32_t errorCode, int32_t extendedErrorCode, download_state state) :
        _bytesTotal(bytesTotal),
        _bytesTransferred(bytesTransferred),
        _errorCode(errorCode),
        _extendedErrorCode(extendedErrorCode),
        _state(state)
    {
    }

    bool is_error() const;

    bool is_transient_error() const;

    bool is_complete() const;

    uint64_t bytes_total() const
    {
        return _bytesTotal;
    }
    
    uint64_t bytes_transferred() const
    {
        return _bytesTransferred;
    }

    int32_t error_code() const
    {
        return _errorCode;
    }

    int32_t extended_error_code() const
    {
        return _extendedErrorCode;
    }

    download_state state() const
    {
        return _state;
    }

private:
    uint64_t        _bytesTotal { 0 };
    uint64_t        _bytesTransferred { 0 };
    int32_t         _errorCode { 0 };
    int32_t         _extendedErrorCode { 0 };
    download_state  _state { download_state::created };

};
}
}
#endif
