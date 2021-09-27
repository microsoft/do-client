// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <chrono>

// Stopwatch implementation using std::chrono::steady_clock.
// Returns elapsed time in milliseconds.
class StopWatch
{
public:
    static StopWatch StartNew();

    void Start();
    void Stop();

    std::chrono::milliseconds GetElapsedInterval() const
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(_GetElapsedTime());
    }

private:
    std::chrono::steady_clock::duration _GetElapsedTime() const;

    std::chrono::steady_clock::time_point _startTime{};
    std::chrono::steady_clock::duration _elapsedTime{ 0 };
    bool _fRunning{ false };
};
