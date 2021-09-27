// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <condition_variable>
#include <chrono>
#include <mutex>
#include <thread>
#include "do_noncopyable.h"
#include "task_queue.h"

class TaskThread : public DONonCopyable
{
public:
    TaskThread();
    ~TaskThread();

    void Unschedule(_In_opt_ const void* tag);

    template <typename TLambda, typename TDuration>
    void Sched(TLambda&& func, TDuration delay, _In_opt_ const void* tag)
    {
        std::unique_lock<std::mutex> lock(_taskQMutex);
        _taskQ.Add(std::forward<TLambda>(func), delay, tag);
        _taskQCond.notify_all();
    }

    template <typename TLambda>
    void Sched(TLambda&& func, _In_opt_ const void* tag = nullptr)
    {
        Sched(std::forward<TLambda>(func), std::chrono::milliseconds(0), tag);
    }

    // Schedules a function on the core thread or executes it if we're already on the core thread
    template <typename TLambda>
    void SchedOrRun(TLambda&& func, _In_opt_ const void* tag)
    {
        if (IsCurrentThread())
        {
            func();
        }
        else
        {
            Sched(std::forward<TLambda>(func), tag);
        }
    }

    // Removes any existing scheduled task with the specified tag and then schedules a new one.
    // Use this instead of calling unschedule() and sched() in sequence to avoid waking up corethread twice.
    template <typename TLambda, typename TDuration>
    void SchedReplace(TLambda&& func, TDuration delay, _In_ const void* tag)
    {
        DO_ASSERT(tag != nullptr);
        std::unique_lock<std::mutex> lock(_taskQMutex);
        _taskQ.Remove(tag);
        _taskQ.Add(std::forward<TLambda>(func), delay, tag);
        _taskQCond.notify_all();
    }

    template <typename TLambda>
    void SchedReplace(TLambda&& func, _In_ const void* tag)
    {
        SchedReplace(std::forward<TLambda>(func), std::chrono::milliseconds(0), tag);
    }

    template <typename TLambda>
    void SchedImmediate(TLambda&& func, _In_opt_ const void* tag = nullptr)
    {
        std::unique_lock<std::mutex> lock(_taskQMutex);
        _taskQ.AddFront(std::forward<TLambda>(func), tag);
        _taskQCond.notify_all();
    }

    // Like schedImmediate but the schedule is optional, i.e., 'func' may not be executed in case of
    // out of memory error while scheduling. Any other error will result in failfast exception.
    template <typename TLambda>
    void SchedImmediateOpt(TLambda&& func, _In_opt_ const void* tag = nullptr) try
    {
        SchedImmediate(std::forward<TLambda>(func), tag);
    } CATCH_LOG()

    void SchedBlock(const std::function<void()>& func, bool immediate = true);

    bool IsScheduled(_In_opt_ const void* tag) const;

    bool IsCurrentThread() const noexcept
        { return ThreadId() == std::this_thread::get_id(); }

    std::thread::id ThreadId() const noexcept
        { return _thread.get_id(); }

private:
    void _DoPoll();

    // A queue of scheduled operations to execute on the core thread
    TaskQueue _taskQ;
    mutable std::mutex _taskQMutex;
    std::condition_variable _taskQCond;

    // Don't init here because _DoPoll depends on _fRunning.
    // Non-trivial init like starting a thread should really be
    // done in the constructor body.
    std::thread _thread;

    bool _fRunning { true };
};
