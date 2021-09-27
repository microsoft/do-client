// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "do_common.h"
#include "stop_watch.h"

StopWatch StopWatch::StartNew()
{
    StopWatch sw;
    sw.Start();
    return sw;
}

void StopWatch::Start()
{
    if (!_fRunning)
    {
        _startTime = std::chrono::steady_clock::now();
        _fRunning = true;
    }
}

void StopWatch::Stop()
{
    if (_fRunning)
    {
        _elapsedTime += std::chrono::steady_clock::now() - _startTime;
        _fRunning = false;
    }
}

std::chrono::steady_clock::duration StopWatch::_GetElapsedTime() const
{
    std::chrono::steady_clock::duration totalElapsedTime = _elapsedTime;
    if (_fRunning)
    {
        auto now = std::chrono::steady_clock::now();
        DO_ASSERT(_startTime <= now);
        totalElapsedTime += (now - _startTime);
    }
    return totalElapsedTime;
}
