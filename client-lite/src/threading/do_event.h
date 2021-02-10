#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>
#include "config_defaults.h"
#include "do_noncopyable.h"

class AutoResetEvent : private DONonCopyable
{
public:
    AutoResetEvent(bool isSignaled = false);

    void SetEvent() noexcept;
    void ResetEvent() noexcept;
    bool IsSignaled() const noexcept;
    bool Wait(std::chrono::milliseconds timeout = g_steadyClockInfiniteWaitTime) noexcept;

private:
    mutable std::mutex _mutex;
    std::condition_variable _cv;
    bool _isSignaled { false };
};

class ManualResetEvent : private DONonCopyable
{
public:
    ManualResetEvent(bool isSignaled = false);

    void SetEvent() noexcept;
    void ResetEvent() noexcept;
    bool IsSignaled() const noexcept;
    bool Wait(std::chrono::milliseconds timeout = g_steadyClockInfiniteWaitTime) noexcept;

private:
    mutable std::mutex _mutex;
    std::condition_variable _cv;
    bool _isSignaled { false };
};
