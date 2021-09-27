// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "do_common.h"
#include "task_thread.h"

#include <memory>
#include "do_event.h"

TaskThread::TaskThread()
{
    _thread = std::thread([this]() { _DoPoll(); });
}

TaskThread::~TaskThread()
{
    SchedImmediate([this]()
    {
        _fRunning = false;
    });
    _thread.join();
}

void TaskThread::Unschedule(_In_opt_ const void* tag)
{
    if (tag != nullptr)
    {
        std::unique_lock<std::mutex> lock(_taskQMutex);
        _taskQ.Remove(tag);
        _taskQCond.notify_all();
    }
}

void TaskThread::_DoPoll()
{
    while (_fRunning)
    {
        std::unique_lock<std::mutex> lock(_taskQMutex);
        const auto next = _taskQ.NextTime();
        const auto now = TaskQueue::clock_t::now();
        if (next <= now)
        {
            std::unique_ptr<TaskQueue::Task> spTask = _taskQ.popNextReady();
            if (spTask)
            {
                lock.unlock();

                spTask->Run();
            }
        }
        else if (next == TaskQueue::timepoint_t::max())
        {
            _taskQCond.wait(lock);
        }
        else
        {
            _taskQCond.wait_for(lock, next - now);
        }
    }

    DoLogInfo("TaskThread exit");
}

void TaskThread::SchedBlock(const std::function<void()>& func, bool immediate)
{
    if (IsCurrentThread())
    {
        func();
        return;
    }

    AutoResetEvent completionEvent;
    HRESULT hr = S_OK;

    auto execOp = [&completionEvent, &func, &hr]()
    {
        try
        {
            func();
        }
        catch (...)
        {
            hr = LOG_CAUGHT_EXCEPTION();
        }
        completionEvent.SetEvent();
    };

    if (immediate)
    {
        SchedImmediate(std::move(execOp));
    }
    else
    {
        Sched(std::move(execOp));
    }

    (void)completionEvent.Wait();

    THROW_IF_FAILED(hr);
}

bool TaskThread::IsScheduled(_In_opt_ const void* tag) const
{
    std::unique_lock<std::mutex> lock(_taskQMutex);
    return _taskQ.Exists(tag);
}
