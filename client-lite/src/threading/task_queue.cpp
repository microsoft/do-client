#include "do_common.h"
#include "task_queue.h"

void TaskQueue::_Add(std::unique_ptr<Task>&& spTask, const duration_t& delay)
{
    const auto adjustedDelay = std::max(duration_t(0), delay);

    auto opTime = clock_t::now() + adjustedDelay;

    _ops.emplace(opTime, std::move(spTask));
}

void TaskQueue::_AddFront(std::unique_ptr<Task>&& spTask)
{
    timepoint_t now = clock_t::now();
    timepoint_t earliest = NextTime();
    timepoint_t opTime;
    if (now < earliest)
    {
        opTime = now - duration_t(1);
    }
    else
    {
        opTime = earliest - duration_t(1);
    }

    _ops.emplace(opTime, std::move(spTask));
}

std::unique_ptr<TaskQueue::Task> TaskQueue::popNextReady(_Out_opt_ const void** tagp)
{
    if (tagp != nullptr)
    {
        *tagp = nullptr;
    }

    std::unique_ptr<Task> rval;
    OpsMap::iterator it = _ops.begin();
    if ((it != _ops.end()) && (it->first <= clock_t::now()))
    {
        rval = std::move(it->second);
        if (tagp)
        {
            *tagp = rval->Tag();
        }
        _ops.erase(it);
    }
    return rval;
}

void TaskQueue::Remove(_In_opt_ const void* tag)
{
    if (tag != nullptr)
    {
        // This is why we want boost::multi_index_container
        OpsMap::iterator it = _ops.begin();
        while (it != _ops.end())
        {
            if (it->second->Tag() == tag)
            {
                it = _ops.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
}

bool TaskQueue::Exists(_In_opt_ const void* tag) const
{
    bool result = false;
    if (tag != nullptr)
    {
        for (const auto& entry : _ops)
        {
            if (entry.second->Tag() == tag)
            {
                result = true;
                break;
            }
        }
    }
    return result;
}

TaskQueue::timepoint_t TaskQueue::NextTime() const
{
    OpsMap::const_iterator it = _ops.begin();
    return (it != _ops.end()) ? it->first : timepoint_t::max();
}