#pragma once

#include <chrono>
#include <memory>
#include "do_file.h"
#include "do_guid.h"
#include "download_progress_tracker.h"
#include "download_status.h"
#include "http_agent_interface.h"
#include "proxy_finder.h"
#include "stop_watch.h"

class MCCManager;
class TaskThread;

// Keep this enum in sync with the full blown DO client in order to not
// have separate mappings in the SDK.
// Only Id, Uri, LocalPath and NoProgressTimeoutSeconds are supported in this client.
enum class DownloadProperty
{
    Id = 0,
    Uri,
    ContentId,
    DisplayName,
    LocalPath,
    HttpCustomHeaders,
    CostPolicy,
    SecurityFlags,
    CallbackFreqPercent,
    CallbackFreqSeconds,
    NoProgressTimeoutSeconds,
    ForegroundPriority,
    BlockingMode,
    CallbackInterface,
    StreamInterface,
    SecurityContext,
    NetworkToken,
    CorrelationVector,
    DecryptionInfo,
    IntegrityCheckInfo,
    IntegrityCheckMandatory,
    TotalSizeBytes,

    Invalid // keep this at the end
};

class Download : public IHttpAgentEvents
{
public:
    // Download(TaskThread& taskThread, REFGUID id); TODO implement along with persistence
    Download(MCCManager& mccManager, TaskThread& taskThread, std::string url = {}, std::string destFilePath = {});
    ~Download();

    void Start();
    void Pause();
    void Finalize();
    void Abort();

    void SetProperty(DownloadProperty key, const std::string& value);
    std::string GetProperty(DownloadProperty key) const;

    DownloadStatus GetStatus() const;
    const GUID& GetId() const { return _id; }
    const std::string& GetUrl() const { return _url; }
    const std::string& GetDestinationPath() const { return _destFilePath; }
    const std::string& GetMCCHost() const { return _mccHost; }
    std::chrono::milliseconds GetElapsedTime() const { return _timer.GetElapsedInterval(); }

    UINT HttpStatusCode() const { return _httpStatusCode; }
    const std::string& ResponseHeaders() const { return _responseHeaders; }
    const DownloadStatus& Status() const { return _status; }

private:
    static const std::chrono::seconds _unsetTimeout;

    MCCManager& _mccManager;
    TaskThread& _taskThread;

    // _fileStream and _httpAgent members are accessed on both the taskthread
    // and http_agent callback thread. See _Pause and _Finalize for special handling.
    // Everything else is accessed only on the taskthread.

    GUID _id;
    std::string _url;
    std::string _destFilePath;
    std::chrono::seconds _noProgressTimeout { _unsetTimeout };

    DownloadStatus _status;
    DownloadProgressTracker _progressTracker;

    StopWatch _timer;

    DOFile _fileStream;
    std::unique_ptr<IHttpAgent> _httpAgent;
    std::string _responseHeaders;
    UINT _httpStatusCode { 0 };
    ProxyList _proxyList;

    // The MCC host name we are using for the current http request, if any
    std::string _mccHost;

    // This flag will indicate whether we have an outstanding http request or not.
    // Need this because we will not move out of Transferring state while waiting before a retry.
    bool _fHttpRequestActive { false };

private:
    void _PerformStateChange(DownloadState newState);
    void _Start();
    void _Resume();
    void _Pause();
    void _Finalize();
    void _Abort();

    void _HandleTransientError(HRESULT hr);
    void _ResumeAfterTransientError();
    void _SendHttpRequest();
    void _SchedProgressTracking();
    void _CancelTasks();

    UINT _MaxNoProgressIntervals() const;

    // IHttpAgentEvents
    HRESULT OnHeadersAvailable(UINT64 httpContext, UINT64) override;
    HRESULT OnData(_In_reads_bytes_(cbData) BYTE* pData, UINT cbData, UINT64, UINT64) override;
    HRESULT OnComplete(HRESULT hResult, UINT64 httpContext, UINT64) override;
};
