#include "do_common.h"
#include "do_curl_multi_operation.h"

#include <algorithm>

CurlMultiOperation::CurlMultiOperation()
{
    _multiHandle = curl_multi_init();
    THROW_HR_IF(E_OUTOFMEMORY, _multiHandle == nullptr);

    _fKeepRunning = true;
    _multiPerformThread = std::thread{[this]()
        {
            _WorkerThread();
        }};
}

CurlMultiOperation::~CurlMultiOperation()
{
    {
        std::unique_lock<std::mutex> lock{_mutex};
        _fKeepRunning = false;
        _cv.notify_one();
    }
    _multiPerformThread.join();

    {
        std::unique_lock<std::mutex> lock{_mutex};
        _activeHandles.RemoveAll(_multiHandle);
        _handlesToAdd.clear();
        _handlesToRemove.clear();
    }

    curl_multi_cleanup(_multiHandle);
}

void CurlMultiOperation::AddHandle(CURL* easyHandle, completion_callback_t pCallback, void* pCallbackUserData)
{
    std::unique_lock<std::mutex> lock{_mutex};
    THROW_HR_IF(E_NOT_VALID_STATE, !_fKeepRunning);
    auto itAdd = std::find(std::begin(_handlesToAdd), std::end(_handlesToAdd), easyHandle);
    THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), itAdd != _handlesToAdd.end());
    _handlesToAdd.emplace_back(HandleData{easyHandle, pCallback, pCallbackUserData});
    _cv.notify_one();
}

void CurlMultiOperation::RemoveHandle(CURL* easyHandle)
{
    std::shared_ptr<WrappedHandleData> wrappedHandleToBeDeleted;
    {
        std::unique_lock<std::mutex> lock{_mutex};

        _RemoveHandle(easyHandle, _handlesToAdd);

        auto pExistingWrappedHandle = _activeHandles.Get(easyHandle);
        if (pExistingWrappedHandle == nullptr)
        {
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

void CurlMultiOperation::_WorkerThread()
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
            _PeformOperations();
        }
    }
}

// Returns when there are no more running handles OR after running for approximately 2s
void CurlMultiOperation::_PeformOperations()
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
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // no relevant handles to poll, short sleep
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
            case -1:
                DO_ASSERT(false); // TODO(shishirb) how to handle?
                break;

            case 0:
            default:
            {
                mc = curl_multi_perform(_multiHandle, &numRunningHandles);
                if ((mc == CURLM_OK) && (numRunningHandles < static_cast<int>(_activeHandles.Size())))
                {
                    _CheckAndHandleCompletedOperations();
                }
            }
        }

        // Return from here periodically to check for exit and handles to be removed
        elapsed = std::chrono::steady_clock::now() - startTime;

    } while ((numRunningHandles > 0) && (elapsed < std::chrono::seconds(2)));
}

void CurlMultiOperation::_CheckAndHandleCompletedOperations()
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

template <typename T>
void CurlMultiOperation::_RemoveHandle(CURL* easyHandle, std::vector<T>& container)
{
    auto itEraseFrom = std::remove(std::begin(container), std::end(container), easyHandle);
    container.erase(itEraseFrom, container.end());
}

void CurlMultiOperation::ActiveHandles::Add(const HandleData& inHandle, CURLM* multiHandle)
{
    if (_Find(inHandle.easyHandle) == _activeHandles.end())
    {
        auto wrappedData = std::make_shared<WrappedHandleData>();
        wrappedData->d = inHandle;
        _activeHandles.emplace_back(std::move(wrappedData));
        curl_multi_add_handle(multiHandle, inHandle.easyHandle);
    }
}

void CurlMultiOperation::ActiveHandles::Remove(CURL* easyHandle, CURLM* multiHandle)
{
    auto itActiveHandle = _Find(easyHandle);
    if (itActiveHandle != _activeHandles.end())
    {
        _Remove(itActiveHandle, multiHandle);
    }
}

void CurlMultiOperation::ActiveHandles::RemoveAll(CURLM* multiHandle)
{
    // The easy handles are not owned here, so no need to call curl_easy_cleanup
    for (const auto& h : _activeHandles)
    {
        (void)curl_multi_remove_handle(multiHandle, h->d.easyHandle);
        h->inactiveSignal.SetEvent();
    }
    _activeHandles.clear();
}

void CurlMultiOperation::ActiveHandles::Complete(CURL* easyHandle, CURLcode result, CURLM* multiHandle)
{
    auto itActiveHandle = _Find(easyHandle);
    DO_ASSERT(itActiveHandle != _activeHandles.end());
    WrappedHandleData& handleData = **itActiveHandle;
    handleData.d.pCallback(result, handleData.d.pCallbackUserData);
    _Remove(itActiveHandle, multiHandle);
}

const std::shared_ptr<CurlMultiOperation::WrappedHandleData>* CurlMultiOperation::ActiveHandles::Get(CURL* eh) const noexcept
{
    auto it = _Find(eh);
    return (it != _activeHandles.end()) ? &(*it) : nullptr;
}

void CurlMultiOperation::ActiveHandles::_Remove(std::vector<std::shared_ptr<WrappedHandleData>>::const_iterator where, CURLM* mh)
{
    auto& h = **where;
    (void)curl_multi_remove_handle(mh, h.d.easyHandle);
    h.inactiveSignal.SetEvent();
    _activeHandles.erase(where);
}
