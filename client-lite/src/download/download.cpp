// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "do_common.h"
#include "download.h"

#include <cmath>
#include "do_cpprest_uri.h"
#include "do_error.h"
#include "event_data.h"
#include "mcc_manager.h"
#include "network_monitor.h"
#include "http_agent.h"
#include "string_ops.h"
#include "task_thread.h"
#include "telemetry_logger.h"

namespace msdod = microsoft::deliveryoptimization::details;

const std::chrono::seconds Download::_unsetTimeout = std::chrono::seconds(0);

static std::string SwapUrlHostNameForMCC(const std::string& url, const std::string& newHostname, UINT16 port = INTERNET_DEFAULT_PORT);

Download::Download(MCCManager& mccManager, TaskThread& taskThread, CurlRequests& curlOps,
        std::string url, std::string destFilePath) :
    _curlOps(curlOps),
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
    _httpAgent = std::make_unique<HttpAgent>(_curlOps, *this);
    _proxyList.Refresh(_url);

    const auto mccFallbackDelay = _mccManager.FallbackDelay();
    if (mccFallbackDelay)
    {
        if (*mccFallbackDelay == g_cacheHostFallbackDelayNoFallback)
        {
            _mccFallbackDue = std::chrono::steady_clock::time_point::max();
        }
        else
        {
            _mccFallbackDue = std::chrono::steady_clock::now() + *mccFallbackDelay;
        }
        DoLogInfo("MCC fallback to original URL throttled for %ld s", mccFallbackDelay->count());
    }

    _mccHost = _mccManager.GetHost();

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
        if (_fAllowMcc)
        {
            _mccHost = _mccManager.GetHost();
        }
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
        if (NetworkMonitor::HasViableInterface())
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

void Download::_SendHttpRequest(bool retryAfterFailure)
{
    const auto& proxy = _proxyList.Next();
    const PCSTR szProxyUrl = !proxy.empty() ? proxy.data() : nullptr;

    const std::string url = _UpdateConnectionTypeAndGetUrl(retryAfterFailure);

    // Default curl connect timeout is too long (300s). Override it.
    // MCC requests get an even shorter timeout because it is expected to be quick and
    // allows faster fallback to original source.
    const UINT connectTimeoutSecs = (_connectionType == ConnectionType::MCC) ? 3 : 15;

    if (_status.BytesTransferred == 0)
    {
        DoLogInfo("%s, requesting full file from %s", GuidToString(_id).data(), url.data());
        THROW_IF_FAILED(_httpAgent->SendRequest(url.data(), szProxyUrl, nullptr, connectTimeoutSecs));
    }
    else
    {
        DO_ASSERT((_status.BytesTotal != 0) && (_status.BytesTransferred < _status.BytesTotal));

        auto range = HttpAgent::MakeRange(_status.BytesTransferred, (_status.BytesTotal - _status.BytesTransferred));
        DoLogInfo("%s, requesting range: %s from %s", GuidToString(_id).data(), range.data(), url.data());
        THROW_IF_FAILED(_httpAgent->SendRequest(url.data(), szProxyUrl, range.data(), connectTimeoutSecs));
    }

    _timer.Start();
    _fHttpRequestActive = true;
    _cbTransferredAtRequestBegin = _status.BytesTransferred;
    if (_connectionType == ConnectionType::CDN)
    {
        _fOriginalHostAttempted = true;
    }

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
        return g_progressTrackerMaxNoProgressIntervals;
    }

    auto maxNoProgressIntervals = std::round(std::chrono::duration<double>(_noProgressTimeout) / g_progressTrackerCheckInterval);
    DO_ASSERT(maxNoProgressIntervals >= 1);
    return static_cast<UINT>(maxNoProgressIntervals);
}

std::string Download::_UpdateConnectionTypeAndGetUrl(bool retryAfterFailure)
{
    auto newConnectionType = _connectionType;
    if (retryAfterFailure)
    {
        DO_ASSERT(_connectionType != ConnectionType::None);

        if (_mccFallbackDue)
        {
            if (*_mccFallbackDue <= std::chrono::steady_clock::now())
            {
                // Fallback time is past due, do not use MCC henceforth
                _fAllowMcc = false;
                newConnectionType = ConnectionType::CDN;
            }
        }
        else
        {
            DO_ASSERT(_fAllowMcc);

            // Fallback config not overriden.
            // Use custom logic to decide when to switch between MCC and CDN.

            const bool noProgressOnCurrentConnectionType = _cbTransferredAtRequestBegin == _status.BytesTransferred;
            if (_connectionType == ConnectionType::MCC)
            {
                if (_fOriginalHostAttempted)
                {
                    if (noProgressOnCurrentConnectionType && (_numAttemptsWithCurrentConnectionType >= 2))
                    {
                        newConnectionType = ConnectionType::CDN;
                    }
                }
                else
                {
                    newConnectionType = ConnectionType::CDN;
                }
            }
            else if (_connectionType == ConnectionType::CDN)
            {
                if (noProgressOnCurrentConnectionType && (_numAttemptsWithCurrentConnectionType >= 2))
                {
                    newConnectionType = ConnectionType::MCC;
                }
            }
            else
            {
                DO_ASSERT(false);
            }
        }
    }
    else
    {
        // Initial start or resume from caller/transient error, attempt with MCC
        newConnectionType = ConnectionType::MCC;
    }

    DO_ASSERT(newConnectionType != ConnectionType::None);

    std::string urlToUse;
    if ((newConnectionType == ConnectionType::MCC) && _fAllowMcc && !_mccHost.empty() && !_mccManager.IsBanned(_mccHost, _url))
    {
        newConnectionType = ConnectionType::MCC;
        urlToUse = SwapUrlHostNameForMCC(_url, _mccHost);
    }
    else
    {
        newConnectionType = ConnectionType::CDN;
        urlToUse = _url;
    }

    if (newConnectionType != _connectionType)
    {
        _connectionType = newConnectionType;
        _numAttemptsWithCurrentConnectionType = 1;
    }
    else
    {
        _numAttemptsWithCurrentConnectionType++;
    }

    DoLogInfo("%s, connection type: %d, numAttempts: %u, url: %s",
        GuidToString(_id).data(), static_cast<int>(_connectionType), _numAttemptsWithCurrentConnectionType, urlToUse.data());
    return urlToUse;
}

bool Download::_ShouldPauseMccUsage(bool isFatalError) const
{
    if (_UsingMcc())
    {
        if (isFatalError)
        {
            return true;
        }

        if (_mccFallbackDue)
        {
            if ((*_mccFallbackDue <= std::chrono::steady_clock::now()))
            {
                // From config override, time to fallback is past due
                return true;
            }
        }
        else
        {
            if (!_fOriginalHostAttempted)
            {
                // Fallback config not overridden and original URL is not yet attempted, fallback now
                return true;
            }
        }

        return false;
    }
    else
    {
        return false;
    }
}

bool Download::_ShouldFailFastPerConnectionType() const
{
    if (_UsingMcc())
    {
        return _NoFallbackFromMcc();
    }
    else
    {
        return true;
    }
}

bool Download::_IsFatalError(HRESULT hrRequest, HRESULT hrCallback, UINT httpStatusCode) const
{
    if (FAILED(hrCallback))
    {
        return true;
    }

    if (HttpAgent::IsClientError(httpStatusCode) && _ShouldFailFastPerConnectionType())
    {
        return true;
    }

    switch (hrRequest)
    {
        case E_OUTOFMEMORY:
        case INET_E_INVALID_URL:
        case DO_E_INSUFFICIENT_RANGE_SUPPORT:
        case HRESULT_FROM_XPLAT_SYSERR(CURLE_WRITE_ERROR):
            return true;
    }

    return false;
}

// IHttpAgentEvents

HRESULT Download::OnHeadersAvailable() try
{
    // Capture relevant data and update internal members asynchronously
    UINT httpStatusCode;
    std::string responseHeaders;
    LOG_IF_FAILED(_httpAgent->QueryStatusCode(&httpStatusCode));
    LOG_IF_FAILED(_httpAgent->QueryHeaders(nullptr, responseHeaders));

    // bytesTotal is required for resume after a pause/error
    UINT64 bytesTotal;
    if (httpStatusCode == HTTP_STATUS_OK)
    {
        RETURN_IF_FAILED(_httpAgent->QueryContentLength(&bytesTotal));
    }
    else if (httpStatusCode == HTTP_STATUS_PARTIAL_CONTENT)
    {
        RETURN_IF_FAILED(_httpAgent->QueryContentLengthFromRange(&bytesTotal));
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

HRESULT Download::OnData(_In_reads_bytes_(cbData) BYTE* pData, UINT cbData) try
{
    _fileStream.Append(pData, cbData);
    _taskThread.Sched([this, cbData]()
    {
        _status.BytesTransferred += cbData;
    }, this);
    return S_OK;
} CATCH_RETURN()

HRESULT Download::OnComplete(HRESULT hrRequest, HRESULT hrCallback)
{
    try
    {
        if (SUCCEEDED(hrRequest))
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
            LOG_IF_FAILED(_httpAgent->QueryStatusCode(&httpStatusCode));
            LOG_IF_FAILED(_httpAgent->QueryHeaders(nullptr, responseHeaders));

            _taskThread.Sched([this, hrRequest, hrCallback, httpStatusCode, responseHeaders = std::move(responseHeaders)]()
            {
                _fHttpRequestActive = false;
                _httpStatusCode = httpStatusCode;
                _responseHeaders = std::move(responseHeaders);

                if (!NetworkMonitor::HasViableInterface())
                {
                    _HandleTransientError(DO_E_BLOCKED_BY_NO_NETWORK);
                    return;
                }

                if (_UsingMcc())
                {
                    _mccManager.ReportHostError(hrRequest, _httpStatusCode, _mccHost, _url);
                }

                const auto hrErrorToReport = FAILED(hrCallback) ? hrCallback : hrRequest;

                // Fail fast on certain http/local errors
                if (_IsFatalError(hrRequest, hrCallback, httpStatusCode))
                {
                    DoLogWarningHr(hrRequest, "%s, fatal failure, http_status: %d, hrCallback: 0x%x, headers:\n%s",
                        GuidToString(_id).data(), _httpStatusCode, hrCallback, _responseHeaders.data());
                    _Pause();
                    _status._Paused(hrErrorToReport);
                    return;
                }

                // Make note of the failure and stay in Transferring state for retry
                _status.Error = hrErrorToReport;
                _progressTracker.OnDownloadFailure();
                std::chrono::seconds retryDelay = _progressTracker.NextRetryDelay();

                // If we must fallback from MCC due to this error, retry without a delay
                if (_ShouldPauseMccUsage(HttpAgent::IsClientError(_httpStatusCode)))
                {
                    retryDelay = std::chrono::seconds(0);
                    _progressTracker.ResetRetryDelay();
                }

                DoLogInfoHr(hrRequest, "%s, failure, will retry in %lld seconds, http_status: %d, hrCallback: 0x%x, headers:\n%s",
                    GuidToString(_id).data(), retryDelay.count(), _httpStatusCode, hrCallback, _responseHeaders.data());
                _taskThread.Sched([this]()
                {
                    // Nothing to do if we moved out of Transferring state in the meantime or
                    // if the http request was already made by a pause-resume cycle.
                    if ((_status.State == DownloadState::Transferring) && !_fHttpRequestActive)
                    {
                        _SendHttpRequest(true);
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
    msdod::cpprest_web::uri inputUri(url);
    msdod::cpprest_web::uri_builder outputUri(inputUri);
    outputUri.set_host(newHostname);
    if (port != INTERNET_DEFAULT_PORT)
    {
        outputUri.set_port(port);
    }
    outputUri.append_query("cacheHostOrigin", inputUri.host());
    return outputUri.to_string();
}
