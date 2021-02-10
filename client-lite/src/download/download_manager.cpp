#include "do_common.h"
#include "download_manager.h"

#include "do_error.h"
#include "download.h"

DownloadManager::DownloadManager(ConfigManager& config) :
    _config(config),
    _mccManager(config)
{
}

std::string DownloadManager::CreateDownload(std::string url, std::string destFilePath)
{
    auto newDownload = std::make_shared<Download>(_mccManager, _taskThread, url, destFilePath);
    const std::string downloadId = newDownload->GetProperty(DownloadProperty::Id);

    std::unique_lock<std::shared_timed_mutex> lock(_downloadsMtx);
    DO_ASSERT(_downloads.find(downloadId) == _downloads.end());
    THROW_HR_IF(DO_E_NO_SERVICE, !_fRunning);
    _downloads.emplace(downloadId, newDownload);
    return downloadId;
}

void DownloadManager::StartDownload(const std::string& downloadId) const
{
    auto download = _GetDownload(downloadId);
    _taskThread.SchedBlock([&download]()
    {
        download->Start();
    });
}

void DownloadManager::PauseDownload(const std::string& downloadId) const
{
    auto download = _GetDownload(downloadId);
    _taskThread.SchedBlock([&download]()
    {
        download->Pause();
    });
}

void DownloadManager::FinalizeDownload(const std::string& downloadId)
{
    auto download = _GetDownload(downloadId);
    _taskThread.SchedBlock([&download]()
    {
        download->Finalize();
    });

    std::unique_lock<std::shared_timed_mutex> lock(_downloadsMtx);
    _downloads.erase(downloadId);
    // TODO(shishirb) remove from _downloads list regardless of whether Finalize succeeded/failed?
}

void DownloadManager::AbortDownload(const std::string& downloadId)
{
    auto download = _GetDownload(downloadId);
    _taskThread.SchedBlock([&download]()
    {
        download->Abort();
    });

    std::unique_lock<std::shared_timed_mutex> lock(_downloadsMtx);
    _downloads.erase(downloadId);
}

void DownloadManager::SetDownloadProperty(const std::string& downloadId, DownloadProperty key, const std::string& value)
{
    auto download = _GetDownload(downloadId);
    _taskThread.SchedBlock([&download, key, &value]()
    {
        download->SetProperty(key, value);
    });
}

std::string DownloadManager::GetDownloadProperty(const std::string& downloadId, DownloadProperty key) const
{
    auto download = _GetDownload(downloadId);
    std::string value;
    _taskThread.SchedBlock([&download, key, &value]()
    {
        value = download->GetProperty(key);
    });
    return value;
}

DownloadStatus DownloadManager::GetDownloadStatus(const std::string& downloadId) const
{
    auto download = _GetDownload(downloadId);

    // Scheduled to the end of the queue in order to get all the updates
    // that might be pending on task thread.
    DownloadStatus status;
    _taskThread.SchedBlock([&download, &status]()
    {
        status = download->GetStatus();
    }, false);
    return status;
}

bool DownloadManager::IsIdle() const
{
    // Reset _fRunning if we are idle to disallow new downloads.
    // This handles the race between shutdown and new download request coming in.
    std::unique_lock<std::shared_timed_mutex> lock(_downloadsMtx);
    _fRunning = !_downloads.empty();
    return !_fRunning;
}

std::shared_ptr<Download> DownloadManager::_GetDownload(const std::string& downloadId) const
{
    std::shared_lock<std::shared_timed_mutex> lock(_downloadsMtx);
    auto it = _downloads.find(downloadId);
    THROW_HR_IF(E_NOT_SET, it == _downloads.end());
    return it->second;
}
