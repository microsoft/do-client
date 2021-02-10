#pragma once

#include <chrono>
#include <shared_mutex>
#include "config_defaults.h"
#include "do_event.h"
#include "do_noncopyable.h"

class WaitableCounter : private DONonCopyable
{
public:
    WaitableCounter() = default;

    ~WaitableCounter()
    {
        // Destruction can happen immediately after _Leave signals _event, which
        // means _lock might not exist when _Leave gets around to releasing it.
        // Acquire the lock here to ensure _Leave can release it safely.
        std::unique_lock<std::shared_timed_mutex> lock(_lock);
    }

    bool Wait(std::chrono::milliseconds timeout = g_steadyClockInfiniteWaitTime) const noexcept
    {
        // Returns true when _count is zero
        return _event.Wait(timeout);
    }

private:
    class scope_exit
    {
    public:
        scope_exit(WaitableCounter* pCounter) : _pCounter(pCounter)
        {
            DO_ASSERT(_pCounter != nullptr);
        }

        // Lambdas in http_agent.cpp require this to be copyable
        scope_exit(const scope_exit& other) : _pCounter(nullptr)
        {
            *this = other;
        }

        scope_exit& operator=(const scope_exit& other)
        {
            _Reset();
            auto counter = other._pCounter;
            if (counter != nullptr)
            {
                *this = std::move(counter->Enter());
            }
            return *this;
        }

        scope_exit(scope_exit&& other) noexcept
        {
            _pCounter = other._pCounter;
            other._pCounter = nullptr;
        }

        scope_exit& operator=(scope_exit&& other) noexcept
        {
            _Reset();
            _pCounter = other._pCounter;
            other._pCounter = nullptr;
            return *this;
        }

        ~scope_exit()
        {
            _Reset();
        }

    private:
        void _Reset() noexcept
        {
            if (_pCounter != nullptr)
            {
                _pCounter->_Leave();
                _pCounter = nullptr;
            }
        }
        WaitableCounter *_pCounter;
    };

public:
    scope_exit Enter() noexcept
    {
        std::unique_lock<std::shared_timed_mutex> lock(_lock);
        ++_count;
        if (_count == 1)
        {
            _event.ResetEvent();
        }
        return scope_exit(this);
    }

private:
    void _Leave() noexcept
    {
        std::unique_lock<std::shared_timed_mutex> lock(_lock);
        DO_ASSERT(_count > 0);
        --_count;
        if (_count == 0)
        {
            _event.SetEvent();
        }
    }

    mutable ManualResetEvent _event { true };
    std::shared_timed_mutex _lock;
    UINT _count { 0 };
};
