#include "do_common.h"
#include "download.h"

#include <cpprest/uri.h>
#include "do_error.h"
#include "event_data.h"
#include "mcc_manager.h"
#include "network_monitor.h"
#include "http_agent.h"
#include "string_ops.h"
#include "task_thread.h"
#include "telemetry_logger.h"

const std::chrono::seconds Download::_unsetTimeout = std::chrono::seconds(0);

static std::string SwapUrlHostNameForMCC(const std::string& url, const std::string& newHostname, UINT16 port = INTERNET_DEFAULT_PORT);

Download::Download(MCCManager& mccManager, TaskThread& taskThread, std::string url, std::string destFilePath) :
    _mccManager(mccManager),
    _taskThread(taskThread),
    _url(std::move(url)),
    _destFilePath(std::move(destFilePath))
{
    if (!_url.empty())
    {
        THROW_HR_IF(INET_E_INVALID_URL, !HttpAgent::ValidateUrl(_url));
    }
    _id = CreateNewGuid();
    DoLogInfo("%s, new download, url: %s, dest: %s", GuidToString(_id).data(), _url.data(), _destFilePath.data());
}

Download::~Download()
{
    _CancelTasks();
}

void Download::Start()
{
    _PerformStateChange(DownloadState::Transferring);

    EventDataDownloadStarted event(*this);
    TelemetryLogger::getInstance().TraceDownloadStart(event);
}

void Download::Pause()
{
    _PerformStateChange(DownloadState::Paused);

    EventDataDownloadPaused event(*this);
    TelemetryLogger::getInstance().TraceDownloadPaused(event);
}

void Download::Finalize()
{
    _PerformStateChange(DownloadState::Finalized);

    EventDataDownloadCompleted event(*this);
    TelemetryLogger::getInstance().TraceDownloadCompleted(event);
}

void Download::Abort()
{
    _PerformStateChange(DownloadState::Aborted);

    EventDataDownloadCanceled event(*this);
    TelemetryLogger::getInstance().TraceDownloadCanceled(event);
}

void Download::SetProperty(DownloadProperty key, const std::string& value)
{
    DoLogInfo("%s, %d = %s", GuidToString(_id).data(), static_cast<int>(key), value.data());
    switch (key)
    {
    case DownloadProperty::Id:
        THROW_HR(DO_E_READ_ONLY_PROPERTY);
        break;

    case DownloadProperty::Uri:
    {
        THROW_HR_IF(E_INVALIDARG, value.empty());
        THROW_HR_IF(INET_E_INVALID_URL, !HttpAgent::ValidateUrl(value));
        THROW_HR_IF(DO_E_INVALID_STATE, (_status.State != DownloadState::Created) && (_status.State != DownloadState::Paused));
        const bool fUrlChanged = (value != _url);
        _url = value;
        if ((_status.State == DownloadState::Paused) && fUrlChanged)
        {
            DoLogInfo("%s, URL changed, reset progress tracker and proxy list", GuidToString(_id).data());
            _progressTracker.Reset();
            _proxyList.Refresh(_url);
        }
        break;
    }

    case DownloadProperty::LocalPath:
        THROW_HR_IF(E_INVALIDARG, value.empty());
        THROW_HR_IF(DO_E_INVALID_STATE, _status.State != DownloadState::Created);
        _destFilePath = value;
        break;

    case DownloadProperty::NoProgressTimeoutSeconds:
    {
        const auto timeout = std::chrono::seconds(docli::string_conversions::ToUInt(value));
        THROW_HR_IF(E_INVALIDARG, timeout < g_progressTrackerCheckInterval);
        _noProgressTimeout = timeout;
        break;
    }

    default:
        DO_ASSERT(false);
        break;
    }
}

std::string Download::GetProperty(DownloadProperty key) const
{
    DoLogInfo("%s, key: %d", GuidToString(_id).data(), static_cast<int>(key));
    switch (key)
    {
    case DownloadProperty::Id:
        return GuidToString(_id);

    case DownloadProperty::Uri:
        return _url;

    case DownloadProperty::LocalPath:
        return _destFilePath;

    case DownloadProperty::NoProgressTimeoutSeconds:
        return std::to_string(_noProgressTimeout.count());

    default:
        DO_ASSERT(false);
        return {};
    }
}

DownloadStatus Download::GetStatus() const
{
    TelemetryLogger::getInstance().TraceDownloadStatus({*this});
    return _status;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
// This method handles state change requests from external caller (through the REST interface)
void Download::_PerformStateChange(DownloadState newState)
{
    DO_ASSERT((newState != DownloadState::Created) && (newState != DownloadState::Transferred));
    DoLogVerbose("%s, state change request %d --> %d", GuidToString(_id).data(), _status.State, newState);
    if (_status.State == DownloadState::Created)
    {
        switch (newState)
        {
        case DownloadState::Transferring:
            _Start();
            _status._Transferring();
            break;

        case DownloadState::Paused: // no-op
            break;

        case DownloadState::Finalized:
            THROW_HR(DO_E_INVALID_STATE);
            break;

        case DownloadState::Aborted:
            _Abort();
            _status._Aborted();
            break;
        }
    }
    else if (_status.State == DownloadState::Transferring)
    {
        switch (newState)
        {
        case DownloadState::Transferring: // no-op
            break;

        case DownloadState::Paused:
            _Pause();
            _status._Paused();
            break;

        case DownloadState::Finalized:
            THROW_HR(DO_E_INVALID_STATE);
            break;

        case DownloadState::Aborted:
            _Abort();
            _status._Aborted();
            break;
        }
    }
    else if (_status.State == DownloadState::Transferred)
    {
        switch (newState)
        {
        case DownloadState::Transferring:
        case DownloadState::Paused:
            break; // no-op

        case DownloadState::Finalized:
            _Finalize();
            _status._Finalized();
            break;

        case DownloadState::Aborted:
            _Abort();
            _status._Aborted();
            break;
        }
    }
    else if (_status.State == DownloadState::Paused)
    {
        switch (newState)
        {
        case DownloadState::Transferring:
            _Resume();
            _status._Transferring();
            break;

        case DownloadState::Paused: // no-op
            break;

        case DownloadState::Finalized:
            THROW_HR(DO_E_INVALID_STATE);
            break;

        case DownloadState::Aborted:
            _Abort();
            _status._Aborted();
            break;
        }
    }
    else if (_status.State == DownloadState::Finalized)
    {
        THROW_HR_IF(DO_E_INVALID_STATE, newState != DownloadState::Finalized);
    }
    else if (_status.State == DownloadState::Aborted)
    {
        THROW_HR_IF(DO_E_INVALID_STATE, newState != DownloadState::Aborted);
    }
    else
    {
        DO_ASSERT(false);
    }
}
#pragma GCC diagnostic pop

void Download::_Start()
{
    THROW_HR_IF(DO_E_DOWNLOAD_NO_URI, _url.empty());
    THROW_HR_IF(DO_E_FILE_DOWNLOADSINK_UNSPECIFIED, _destFilePath.empty());

    _fileStream = DOFile::Create(_destFilePath);
    _fDestFileCreated = true;
    _httpAgent = std::make_unique<HttpAgent>(*this);
    _proxyList.Refresh(_url);

    DO_ASSERT((_status.BytesTotal == 0) && (_status.BytesTransferred == 0));
    _SendHttpRequest();
}

void Download::_Resume()
{
    DO_ASSERT(_httpAgent);
    // BytesTotal can be zero if the start request never completed due to an error/pause
    DO_ASSERT((_status.BytesTotal != 0) || (_status.BytesTransferred == 0));

    _fileStream = DOFile::Open(_destFilePath);

    if ((_status.BytesTotal != 0) && (_status.BytesTransferred == _status.BytesTotal))
    {
        // We could get here if download was Paused just as we wrote the last block in OnData and scheduled the status update.
        // That would have caused OnComplete to not get called so now we discover that the download was completed.
        // Schedule the state update asynchronously here to account for the state change that is done upon returning from here.
        DoLogInfo("%s, already transferred %llu out of %llu bytes", GuidToString(_id).data(), _status.BytesTransferred, _status.BytesTotal);
        _taskThread.SchedImmediate([this]()
        {
            _status._Transferred();
        }, this);
    }
    else
    {
        _SendHttpRequest();
    }
}

void Download::_Pause()
{
    _httpAgent->Close();    // waits until all callbacks are complete
    _timer.Stop();
    _fHttpRequestActive = false;
    _fileStream.Close();   // safe to close now that no callbacks are expected
}

void Download::_Finalize()
{
    _httpAgent->Close();    // waits until all callbacks are complete
    _fHttpRequestActive = false;
    _fileStream.Close();    // safe since no callbacks are expected
    _CancelTasks();
}

void Download::_Abort() try
{
    if (_httpAgent)
    {
        _httpAgent->Close();
    }
    _timer.Stop();
    _fileStream.Close();
    _CancelTasks();
    // Delete file only if this download is the creator/owner. The abort could be from an
    // error downloading to an already existing file.
    if (_fDestFileCreated && !_destFilePath.empty())
    {
        DOFile::Delete(_destFilePath);
    }
} CATCH_LOG()

void Download::_HandleTransientError(HRESULT hr)
{
    DO_ASSERT(FAILED(hr));

    // Transition into transient error state and schedule retry
    const auto retryDelay = std::chrono::seconds(30);
    DoLogInfo("%s, transient error: %x, will retry in %lld seconds", GuidToString(_id).data(), hr, retryDelay.count());
    _status._Paused(S_OK, hr);
    _taskThread.Sched([this]()
    {
        if (NetworkMonitor::IsConnected())
        {
            _ResumeAfterTransientError();
        }
        else
        {
            // Still no network connectivity, retry again after delay
            _HandleTransientError(DO_E_BLOCKED_BY_NO_NETWORK);
        }
    }, retryDelay, this);
}

void Download::_ResumeAfterTransientError()
{
    DoLogInfo("%s, state: %d, error: %x, ext_error: %x", GuidToString(_id).data(), _status.State, _status.Error, _status.ExtendedError);
    if (_status.IsTransientError())
    {
        DO_ASSERT(!_fHttpRequestActive);
        _SendHttpRequest();
        _status._Transferring();
    }
    // else nothing to do since we are not in transient error state
}

void Download::_SendHttpRequest()
{
    const auto& proxy = _proxyList.Next();
    const PCSTR szProxyUrl = !proxy.empty() ? proxy.data() : nullptr;

    std::string url = _url;
    std::string mccHost = _mccManager.NextHost();
    if (!mccHost.empty())
    {
        DoLogInfo("Found MCC host: %s", mccHost.data());
        url = SwapUrlHostNameForMCC(_url, mccHost);
    }

    if ((_status.State != DownloadState::Created) && (mccHost != _mccHost))
    {
        DoLogInfo("%s, MCC host changed, reset progress tracker", GuidToString(_id).data());
        _progressTracker.Reset();
    }

    if (_status.BytesTransferred == 0)
    {
        DoLogInfo("%s, requesting full file from %s", GuidToString(_id).data(), url.data());
        THROW_IF_FAILED(_httpAgent->SendRequest(url.data(), szProxyUrl));
    }
    else
    {
        DO_ASSERT((_status.BytesTotal != 0) && (_status.BytesTransferred < _status.BytesTotal));

        auto range = HttpAgent::MakeRange(_status.BytesTransferred, (_status.BytesTotal - _status.BytesTransferred));
        DoLogInfo("%s, requesting range: %s from %s", GuidToString(_id).data(), range.data(), url.data());
        THROW_IF_FAILED(_httpAgent->SendRequest(url.data(), szProxyUrl, nullptr, range.data()));
    }

    _timer.Start();
    _fHttpRequestActive = true;
    _mccHost = std::move(mccHost);

    // Clear error codes, they will get updated once the request completes
    _status.Error = S_OK;
    _status.ExtendedError = S_OK;

    _SchedProgressTracking();
}

void Download::_SchedProgressTracking()
{
    if (_taskThread.IsScheduled(&_progressTracker))
    {
        return;
    }

    _taskThread.Sched([this]()
    {
        if (_status.State == DownloadState::Transferring)
        {
            const bool fTimedOut = _progressTracker.CheckProgress(_status.BytesTransferred, _MaxNoProgressIntervals());
            if (fTimedOut)
            {
                _Pause();
                _status._Paused(DO_E_DOWNLOAD_NO_PROGRESS, _status.Error);
            }
            else
            {
                _SchedProgressTracking();
            }
        }
    }, g_progressTrackerCheckInterval, &_progressTracker);
}

void Download::_CancelTasks()
{
    _taskThread.Unschedule(this);
    _taskThread.Unschedule(&_progressTracker);
}

UINT Download::_MaxNoProgressIntervals() const
{
    if (_noProgressTimeout == _unsetTimeout)
    {
        if (!_mccHost.empty() && _mccManager.NoFallback() && HttpAgent::IsClientError(_httpStatusCode))
        {
            // Special case for MCC without CDN fallback and 4xx errors
            return g_progressTrackerMaxNoProgressIntervalsWithMCC;
        }
        else
        {
            return g_progressTrackerMaxNoProgressIntervals;
        }
    }

    auto maxNoProgressIntervals = std::round(std::chrono::duration<double>(_noProgressTimeout) / g_progressTrackerCheckInterval);
    DO_ASSERT(maxNoProgressIntervals >= 1);
    return static_cast<UINT>(maxNoProgressIntervals);
}

// IHttpAgentEvents

HRESULT Download::OnHeadersAvailable(UINT64 httpContext, UINT64) try
{
    // Capture relevant data and update internal members asynchronously
    UINT httpStatusCode;
    std::string responseHeaders;
    LOG_IF_FAILED(_httpAgent->QueryStatusCode(httpContext, &httpStatusCode));
    LOG_IF_FAILED(_httpAgent->QueryHeaders(httpContext, nullptr, responseHeaders));

    // bytesTotal is required for resume after a pause/error
    UINT64 bytesTotal;
    if (httpStatusCode == HTTP_STATUS_OK)
    {
        RETURN_IF_FAILED(_httpAgent->QueryContentLength(httpContext, &bytesTotal));
    }
    else if (httpStatusCode == HTTP_STATUS_PARTIAL_CONTENT)
    {
        RETURN_IF_FAILED(_httpAgent->QueryContentLengthFromRange(httpContext, &bytesTotal));
    }

    DoLogInfo("%s, http_status: %d, content_length: %llu, headers:\n%s",
        GuidToString(_id).data(), httpStatusCode, bytesTotal, responseHeaders.data());

    _taskThread.Sched([this, httpStatusCode, bytesTotal, responseHeaders = std::move(responseHeaders)]()
    {
        _httpStatusCode = httpStatusCode;
        _responseHeaders = std::move(responseHeaders);
        _status.BytesTotal = bytesTotal;
    }, this);

    return S_OK;
} CATCH_RETURN()

HRESULT Download::OnData(_In_reads_bytes_(cbData) BYTE* pData, UINT cbData, UINT64, UINT64) try
{
    _fileStream.Append(pData, cbData);
    _taskThread.Sched([this, cbData]()
    {
        _status.BytesTransferred += cbData;
    }, this);
    return S_OK;
} CATCH_RETURN()

HRESULT Download::OnComplete(HRESULT hResult, UINT64 httpContext, UINT64)
{
    try
    {
        if (SUCCEEDED(hResult))
        {
            _taskThread.Sched([this]()
            {
                _fHttpRequestActive = false;
                _timer.Stop();
                _status._Transferred();
            }, this);
        }
        else
        {
            // OnHeadersAvailable might not have been called in the failure case depending on
            // when the failure occurred - upon connecting, or while reading response data.
            UINT httpStatusCode;
            std::string responseHeaders;
            LOG_IF_FAILED(_httpAgent->QueryStatusCode(httpContext, &httpStatusCode));
            LOG_IF_FAILED(_httpAgent->QueryHeaders(httpContext, nullptr, responseHeaders));

            _taskThread.Sched([this, hResult, httpStatusCode, responseHeaders = std::move(responseHeaders)]()
            {
                _fHttpRequestActive = false;
                _httpStatusCode = httpStatusCode;
                _responseHeaders = std::move(responseHeaders);

                if (!NetworkMonitor::IsConnected())
                {
                    _HandleTransientError(DO_E_BLOCKED_BY_NO_NETWORK);
                    return;
                }

                // Fail fast on certain http errors if we are not using MCC.
                // Logic differs slightly when MCC is used. See _MaxNoProgressIntervals().
                if (_mccHost.empty() && HttpAgent::IsClientError(_httpStatusCode))
                {
                    DoLogInfoHr(hResult, "%s, fatal failure, http_status: %d, headers:\n%s",
                        GuidToString(_id).data(), _httpStatusCode, _responseHeaders.data());
                    _Pause();
                    _status._Paused(hResult);
                    return;
                }

                // Make note of the failure and stay in Transferring state for retry
                _status.Error = hResult;
                _progressTracker.OnDownloadFailure();
                std::chrono::seconds retryDelay = _progressTracker.NextRetryDelay();

                // Report error to MCC manager if MCC was used
                if (!_mccHost.empty())
                {
                    // We were using MCC. If it is time to fallback to original URL, it should happen without delay.
                    const bool isFallbackDue = _mccManager.ReportHostError(hResult, _mccHost);
                    if (isFallbackDue)
                    {
                        retryDelay = std::chrono::seconds(0);
                    }
                }

                DoLogInfoHr(hResult, "%s, failure, will retry in %lld seconds, http_status: %d, headers:\n%s",
                    GuidToString(_id).data(), retryDelay.count(), _httpStatusCode, _responseHeaders.data());
                _taskThread.Sched([this]()
                {
                    // Nothing to do if we moved out of Transferring state in the meantime or
                    // if the http request was already made by a pause-resume cycle.
                    if ((_status.State == DownloadState::Transferring) && !_fHttpRequestActive)
                    {
                        _SendHttpRequest();
                    }
                }, retryDelay, this);
            }, this);
        }
    } CATCH_LOG()
    return S_OK;
}

std::string SwapUrlHostNameForMCC(const std::string& url, const std::string& newHostname, UINT16 port)
{
    // Switch the hostname and add the original hostname as a query param
    web::uri inputUri(url);
    web::uri_builder outputUri(inputUri);
    outputUri.set_host(newHostname);
    if (port != INTERNET_DEFAULT_PORT)
    {
        outputUri.set_port(port);
    }
    outputUri.append_query("cacheHostOrigin", inputUri.host());
    return outputUri.to_string();
}
