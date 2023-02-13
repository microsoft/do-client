// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _DELIVERY_OPTIMIZATION_DO_DOWNLOAD_STATUS_H
#define _DELIVERY_OPTIMIZATION_DO_DOWNLOAD_STATUS_H

#include <cstdint>
#include <functional>
#include <system_error>

#include "do_errors.h"

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

    download_status(uint64_t bytesTotal, uint64_t bytesTransferred, int32_t errorCode, int32_t extendedErrorCode, download_state state) noexcept :
        _bytesTotal(bytesTotal),
        _bytesTransferred(bytesTransferred),
        _errorCode(errorCode),
        _extendedErrorCode(extendedErrorCode),
        _state(state)
    {
    }

    bool is_error() const noexcept;

    bool is_transient_error() const noexcept;

    bool is_complete() const noexcept;

    uint64_t bytes_total() const noexcept
    {
        return _bytesTotal;
    }

    uint64_t bytes_transferred() const noexcept
    {
        return _bytesTransferred;
    }

    std::error_code error_code() const noexcept;
    std::error_code extended_error_code() const noexcept;

    download_state state() const noexcept
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

class download;
using status_callback_t = std::function<void(download&, download_status&)>;

// Future: use std::span (requires C++20)
using output_stream_callback_t = std::function<std::error_code(const unsigned char* data, size_t size)>;

// TODO: create a new header for things that aren't 'status' related, such as output_stream_callback_t and ranges
struct download_range
{
    uint64_t offset;
    uint64_t length;
};
static constexpr uint64_t length_to_eof = static_cast<uint64_t>(-1);

} // namespace deliveryoptimization
} // namespace microsoft

#endif // _DELIVERY_OPTIMIZATION_DO_DOWNLOAD_STATUS_H
