#pragma once

#include <chrono>
#include "config_defaults.h"

class DownloadProgressTracker
{
public:
    bool CheckProgress(UINT64 newBytesTransferred, UINT maxNoProgressIntervals = g_progressTrackerMaxNoProgressIntervals)
    {
        DO_ASSERT(newBytesTransferred >= _lastSeenBytesTransferred);

        if (newBytesTransferred > _lastSeenBytesTransferred)
        {
            _numNoProgressIntervals = 0;
            _lastSeenBytesTransferred = newBytesTransferred;
        }
        else
        {
            _numNoProgressIntervals++;
        }
        DoLogInfo("Bytes transferred so far: %llu, no-progress intervals: [cur %u, max %u]",
            _lastSeenBytesTransferred, _numNoProgressIntervals, maxNoProgressIntervals);
        return (_numNoProgressIntervals >= maxNoProgressIntervals);
    }

    void OnDownloadFailure()
    {
        _nextRetryDelay = std::min(g_progressTrackerMaxRetryDelay, _nextRetryDelay * 2);
    }

    // Forget the count of no-progress until now, retry delay will start from minimum next time
    void Reset()
    {
        _nextRetryDelay = std::chrono::seconds(1);
        _numNoProgressIntervals = 0;
    }

    auto NextRetryDelay() const
    {
        return _nextRetryDelay;
    }

private:
    std::chrono::seconds _nextRetryDelay { 1 };
    UINT _numNoProgressIntervals { 0 };
    UINT64 _lastSeenBytesTransferred { 0 };
};
