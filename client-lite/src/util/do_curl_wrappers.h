// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <curl/curl.h>
#include "do_event.h"

class CurlGlobalInit
{
public:
    CurlGlobalInit()
    {
        auto result = curl_global_init(CURL_GLOBAL_ALL);
        THROW_HR_IF(E_OUTOFMEMORY, result != CURLE_OK);
    }

    ~CurlGlobalInit()
    {
        curl_global_cleanup();
    }

    CurlGlobalInit(const CurlGlobalInit&) = delete;
    CurlGlobalInit& operator=(const CurlGlobalInit&) = delete;
    CurlGlobalInit(CurlGlobalInit&&) noexcept = delete;
    CurlGlobalInit& operator=(CurlGlobalInit&&) noexcept = delete;
};

class CurlRequests
{
public:
    using completion_callback_t = void (*)(int, void*);

    CurlRequests();
    ~CurlRequests();

    void Add(CURL* easyHandle, completion_callback_t pCallback, void* pCallbackUserData);
    void Remove(CURL* easyHandle);

private:
    struct HandleData
    {
        CURL* easyHandle;
        completion_callback_t pCallback;
        void* pCallbackUserData;

        bool operator==(CURL* eh) const noexcept { return easyHandle && (easyHandle == eh); }
    };

    struct WrappedHandleData
    {
        HandleData d;
        ManualResetEvent inactiveSignal;

        bool operator==(CURL* eh) const noexcept { return d.easyHandle && (d.easyHandle == eh); }
    };

    class ActiveHandles
    {
    private:
        std::vector<std::shared_ptr<WrappedHandleData>> _handles;

        void _Remove(std::vector<std::shared_ptr<WrappedHandleData>>::const_iterator where, CURLM* mh);

        auto _Find(CURL* eh) const noexcept
        {
            return std::find_if(std::begin(_handles), std::end(_handles), [eh](const auto& ah)
                {
                    return *ah == eh;
                });
        }

    public:
        void Add(const HandleData& inHandle, CURLM* multiHandle);
        void Remove(CURL* easyHandle, CURLM* multiHandle);
        void RemoveAll(CURLM* multiHandle);
        void Complete(CURL* easyHandle, CURLcode result, CURLM* multiHandle);

        const std::shared_ptr<WrappedHandleData>* Get(CURL* eh) const noexcept;
        auto Empty() const noexcept { return _handles.empty(); }
        auto Size() const noexcept { return _handles.size(); }
    };

    void _DoWork();
    void _PerformTransferTasks();
    void _CheckForAndHandleCompletedRequestsUnderLock();

    CURLM* _multiHandle { nullptr };

    std::vector<HandleData> _handlesToAdd;
    std::vector<CURL*> _handlesToRemove;
    ActiveHandles _activeHandles;

    std::thread _multiPerformThread;
    std::mutex _mutex;
    std::condition_variable _cv;
    bool _fKeepRunning { false };
};
