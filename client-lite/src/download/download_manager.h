#pragma once

#include <shared_mutex>
#include <unordered_map>
#include "mcc_manager.h"
#include "task_thread.h"

enum class DownloadProperty;
class Download;
struct DownloadStatus;

class DownloadManager
{
    friend std::shared_ptr<Download> DownloadForId(const DownloadManager& manager, const std::string& id);

public:
    DownloadManager(ConfigManager& config);

    std::string CreateDownload(std::string url = {}, std::string destFilePath = {});

    void StartDownload(const std::string& downloadId) const;
    void PauseDownload(const std::string& downloadId) const;
    void FinalizeDownload(const std::string& downloadId);
    void AbortDownload(const std::string& downloadId);
    void SetDownloadProperty(const std::string& downloadId, DownloadProperty key, const std::string& value);
    std::string GetDownloadProperty(const std::string& downloadId, DownloadProperty key) const;
    DownloadStatus GetDownloadStatus(const std::string& downloadId) const;

    bool IsIdle() const;
    void RefreshConfigs() const;

private:
    mutable TaskThread _taskThread;
    std::unordered_map<std::string, std::shared_ptr<Download>> _downloads;
    mutable bool _fRunning { true };
    mutable std::shared_timed_mutex _downloadsMtx;

    ConfigManager& _config;
    MCCManager _mccManager;

private:
    std::shared_ptr<Download> _GetDownload(const std::string& downloadId) const;
};
