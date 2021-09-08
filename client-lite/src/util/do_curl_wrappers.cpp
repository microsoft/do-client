#include "do_common.h"
#include "do_curl_wrappers.h"

#include <algorithm>

CurlRequests::CurlRequests()
{
    _multiHandle = curl_multi_init();
    THROW_HR_IF(E_OUTOFMEMORY, _multiHandle == nullptr);

    _fKeepRunning = true;
    _multiPerformThread = std::thread{[this]()
        {
            _DoWork();
        }};
}

CurlRequests::~CurlRequests()
{
    {
        std::unique_lock<std::mutex> lock{_mutex};
        _fKeepRunning = false;
        _cv.notify_one();
    }
    _multiPerformThread.join();

    {
        _activeHandles.RemoveAll(_multiHandle);
        _handlesToAdd.clear();
        _handlesToRemove.clear();
    }

    curl_multi_cleanup(_multiHandle);
}

void CurlRequests::Add(CURL* easyHandle, completion_callback_t pCallback, void* pCallbackUserData)
{
    std::unique_lock<std::mutex> lock{_mutex};
    THROW_HR_IF(E_NOT_VALID_STATE, !_fKeepRunning);
    auto itAdd = std::find(std::begin(_handlesToAdd), std::end(_handlesToAdd), easyHandle);
    THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), itAdd != _handlesToAdd.end());
    _handlesToAdd.emplace_back(HandleData{easyHandle, pCallback, pCallbackUserData});
    _cv.notify_one();
}

void CurlRequests::Remove(CURL* easyHandle)
{
    std::shared_ptr<WrappedHandleData> wrappedHandleToBeDeleted;
    {
        std::unique_lock<std::mutex> lock{_mutex};

        _handlesToAdd.erase(std::remove(std::begin(_handlesToAdd), std::end(_handlesToAdd), easyHandle), _handlesToAdd.end());

        auto pExistingWrappedHandle = _activeHandles.Get(easyHandle);
        if (pExistingWrappedHandle == nullptr)
        {
            // Handle not in active list, nothing more to do
            return;
        }
        wrappedHandleToBeDeleted = *pExistingWrappedHandle;

        if (std::find(std::begin(_handlesToRemove), std::end(_handlesToRemove), easyHandle) == _handlesToRemove.end())
        {
            _handlesToRemove.emplace_back(easyHandle);
        }
        _cv.notify_one();
    }

    if (wrappedHandleToBeDeleted)
    {
        // outside the lock, wait for the handle to be removed from _activeHandles
        (void)wrappedHandleToBeDeleted->inactiveSignal.Wait(std::chrono::minutes(30));
    }
}

void CurlRequests::_DoWork()
{
    while (true)
    {
        std::unique_lock<std::mutex> lock{_mutex};
        if (!_fKeepRunning)
        {
            break;
        }

        for (const auto& h : _handlesToAdd)
        {
            _activeHandles.Add(h, _multiHandle);
        }
        _handlesToAdd.clear();

        for (const auto& h : _handlesToRemove)
        {
            _activeHandles.Remove(h, _multiHandle);
        }
        _handlesToRemove.clear();

        if (_activeHandles.Empty())
        {
            _cv.wait(lock, [this](){ return !_fKeepRunning || !_handlesToAdd.empty(); });
        }
        else
        {
            lock.unlock();
            _PerformTransferTasks();
        }
    }
}

// Returns when there are no more running handles OR after running for approximately 2s
void CurlRequests::_PerformTransferTasks()
{
    int numRunningHandles = 0;
    const auto startTime = std::chrono::steady_clock::now();
    std::chrono::steady_clock::duration elapsed {};
    do
    {
        long curlTimeoutMsecs;
        curl_multi_timeout(_multiHandle, &curlTimeoutMsecs);
        if (curlTimeoutMsecs < 0)
        {
            curlTimeoutMsecs = 1000;
        }

        fd_set fdsRead;
        fd_set fdsWrite;
        fd_set fdsException;
        int maxfd = -1;
        FD_ZERO(&fdsRead);
        FD_ZERO(&fdsWrite);
        FD_ZERO(&fdsException);

        int selectResult;
        CURLMcode mc = curl_multi_fdset(_multiHandle, &fdsRead, &fdsWrite, &fdsException, &maxfd);
        if (maxfd == -1)
        {
            // no relevant handles to poll, short sleep
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            selectResult = 0;
        }
        else
        {
            struct timeval timeout;
            timeout.tv_sec = curlTimeoutMsecs / 1000;
            timeout.tv_usec = (curlTimeoutMsecs % 1000) * 1000;
            selectResult = select(maxfd + 1, &fdsRead, &fdsWrite, &fdsException, &timeout);
        }

        switch (selectResult)
        {
            // TODO(shishirb) Should we do anything other than retry?
            case -1:
                DO_ASSERT(false);
                break;

            // select() timed out or we did the short sleep above.
            // Fallthrough and call curl_multi_perform even in this case to let
            // libcurl perform internal retries and timeouts.
            case 0:

            // Default case is select() indicating there are one or more sockets ready for reading/writing
            default:
            {
                mc = curl_multi_perform(_multiHandle, &numRunningHandles);
                if (mc == CURLM_OK)
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    if (numRunningHandles < static_cast<int>(_activeHandles.Size()))
                    {
                        // One or more handles have completed, check and invoke callbacks
                        _CheckForAndHandleCompletedRequestsUnderLock();
                    }
                }
            }
        }

        // Return from here periodically to check for exit and handles to be removed
        elapsed = std::chrono::steady_clock::now() - startTime;

    } while ((numRunningHandles > 0) && (elapsed < std::chrono::seconds(2)));
}

void CurlRequests::_CheckForAndHandleCompletedRequestsUnderLock()
{
    struct CURLMsg* msg;
    do
    {
        int msgQ = 0;
        msg = curl_multi_info_read(_multiHandle, &msgQ);
        if (msg && (msg->msg == CURLMSG_DONE))
        {
            _activeHandles.Complete(msg->easy_handle, msg->data.result, _multiHandle);
        }
    } while (msg != nullptr);
}

void CurlRequests::ActiveHandles::Add(const HandleData& inHandle, CURLM* multiHandle)
{
    if (_Find(inHandle.easyHandle) == _handles.end())
    {
        auto wrappedData = std::make_shared<WrappedHandleData>();
        wrappedData->d = inHandle;
        _handles.emplace_back(std::move(wrappedData));
        curl_multi_add_handle(multiHandle, inHandle.easyHandle);
    }
}

void CurlRequests::ActiveHandles::Remove(CURL* easyHandle, CURLM* multiHandle)
{
    auto itActiveHandle = _Find(easyHandle);
    if (itActiveHandle != _handles.end())
    {
        _Remove(itActiveHandle, multiHandle);
    }
}

void CurlRequests::ActiveHandles::RemoveAll(CURLM* multiHandle)
{
    // The easy handles are not owned here, so no need to call curl_easy_cleanup
    for (const auto& h : _handles)
    {
        (void)curl_multi_remove_handle(multiHandle, h->d.easyHandle);
        h->inactiveSignal.SetEvent();
    }
    _handles.clear();
}

void CurlRequests::ActiveHandles::Complete(CURL* easyHandle, CURLcode result, CURLM* multiHandle)
{
    auto itActiveHandle = _Find(easyHandle);
    DO_ASSERT(itActiveHandle != _handles.end());
    WrappedHandleData& handleData = **itActiveHandle;
    handleData.d.pCallback(result, handleData.d.pCallbackUserData);
    _Remove(itActiveHandle, multiHandle);
}

const std::shared_ptr<CurlRequests::WrappedHandleData>* CurlRequests::ActiveHandles::Get(CURL* eh) const noexcept
{
    auto it = _Find(eh);
    return (it != _handles.end()) ? &(*it) : nullptr;
}

void CurlRequests::ActiveHandles::_Remove(std::vector<std::shared_ptr<WrappedHandleData>>::const_iterator where, CURLM* mh)
{
    auto& h = **where;
    (void)curl_multi_remove_handle(mh, h.d.easyHandle);
    h.inactiveSignal.SetEvent();
    _handles.erase(where);
}
