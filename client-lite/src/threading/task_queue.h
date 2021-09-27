// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <chrono>
#include <map>
#include <memory>

// Note: Locking is left to the queue's owner
class TaskQueue
{
public:
    using clock_t = std::chrono::steady_clock;
    using timepoint_t = clock_t::time_point;
    using duration_t = std::chrono::milliseconds;

    class Task
    {
    public:
        virtual ~Task() {}
        virtual void Run() = 0;
        virtual const void* Tag() = 0;
    };

private:
    template <typename TLambda>
    class Op : public Task, std::remove_reference_t<TLambda>
    {
        Op(const Op&) = delete;
        Op& operator=(const Op&) = delete;

    public:
        Op(TLambda&& lambda, const void* tag) :
            std::remove_reference_t<TLambda>(std::forward<TLambda>(lambda)),
            _tag(tag)
        {
        }

        void Run() override
        {
            (*this)();
        }

        const void* Tag() override
        {
            return _tag;
        }

    private:
        const void* _tag;
    };

    void _Add(std::unique_ptr<Task>&& spTask, const duration_t& delay);
    void _AddFront(std::unique_ptr<Task>&& spTask);

    typedef std::multimap<timepoint_t, std::unique_ptr<Task>> OpsMap;
    OpsMap _ops;

public:
    // Note: If tag == nullptr, the op can't be removed. remove(nullptr) is a no-op.
    template <typename TLambda, typename TDuration>
    void Add(TLambda&& func, TDuration delay, _In_opt_ const void* tag = nullptr)
    {
        _Add(std::make_unique<Op<TLambda>>(std::forward<TLambda>(func), tag), std::chrono::duration_cast<duration_t>(delay));
    }

    template <typename TLambda>
    void AddFront(TLambda&& func, _In_opt_ const void* tag = nullptr)
    {
        _AddFront(std::make_unique<Op<TLambda>>(std::forward<TLambda>(func), tag));
    }

    std::unique_ptr<Task> popNextReady(_Out_opt_ const void** tagp = nullptr);
    void Remove(_In_opt_ const void* tag);
    bool Exists(_In_opt_ const void* tag) const;

    timepoint_t NextTime() const;
};
