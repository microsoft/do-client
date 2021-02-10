#include "do_common.h"
#include "do_event.h"

#include <chrono>

#ifdef DEBUG
inline void _VerifyWaitTime(std::chrono::milliseconds timeout)
{
    // Wait time must be small enough to not overflow when added to now()
    const auto now = std::chrono::steady_clock::now();
    DO_ASSERT((now + (timeout)) >= now);
}
#else
#define _VerifyWaitTime(t)
#endif

// AutoResetEvent

AutoResetEvent::AutoResetEvent(bool isSignaled) :
    _isSignaled(isSignaled)
{
}

void AutoResetEvent::SetEvent() noexcept
{
    std::lock_guard<std::mutex> lock(_mutex);
    _isSignaled = true;
    _cv.notify_one();
}

void AutoResetEvent::ResetEvent() noexcept
{
    std::lock_guard<std::mutex> lock(_mutex);
    _isSignaled = false;
}

bool AutoResetEvent::IsSignaled() const noexcept
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _isSignaled;
}

bool AutoResetEvent::Wait(std::chrono::milliseconds timeout) noexcept
{
    _VerifyWaitTime(timeout);

    std::unique_lock<std::mutex> lock(_mutex);
    if (!_isSignaled)
    {
        if (!_cv.wait_for(lock, timeout, [this] { return _isSignaled; }))
        {
            return false;
        }
    }
    _isSignaled = false;
    return true;
}

// ManualResetEvent

ManualResetEvent::ManualResetEvent(bool isSignaled) :
    _isSignaled(isSignaled)
{
}

void ManualResetEvent::SetEvent() noexcept
{
    std::lock_guard<std::mutex> lock(_mutex);
    _isSignaled = true;
    _cv.notify_all();
}

void ManualResetEvent::ResetEvent() noexcept
{
    std::lock_guard<std::mutex> lock(_mutex);
    _isSignaled = false;
}

bool ManualResetEvent::IsSignaled() const noexcept
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _isSignaled;
}

bool ManualResetEvent::Wait(std::chrono::milliseconds timeout) noexcept
{
    _VerifyWaitTime(timeout);

    std::unique_lock<std::mutex> lock(_mutex);
    if (!_isSignaled)
    {
        if (!_cv.wait_for(lock, timeout, [this] { return _isSignaled; }))
        {
            return false;
        }
    }
    return true;
}
