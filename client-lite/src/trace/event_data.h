#pragma once

#include <chrono>
#include <string>

#include "do_guid.h"
#include "download_status.h"

class Download;

struct TelDataDownloadInfo
{
    TelDataDownloadInfo(const Download& download);
    TelDataDownloadInfo() = default;

    GUID _guid;
    DownloadStatus _status;
    std::string _url;
    std::string _destinationPath;
    std::string _mccHost;
};

struct EventDataDownloadStarted
{
    EventDataDownloadStarted(const Download& download);

    TelDataDownloadInfo _commonData;
};

struct EventDataDownloadCompleted
{
    EventDataDownloadCompleted(const Download& download);

    TelDataDownloadInfo _commonData;
    std::chrono::milliseconds _elapsedTime;
};

struct EventDataDownloadPaused
{
    EventDataDownloadPaused(const Download& download);

    TelDataDownloadInfo _commonData;
};

struct EventDataDownloadCanceled
{
    EventDataDownloadCanceled(const Download& download);

    TelDataDownloadInfo _commonData;
};

struct EventDataDownloadStatus
{
    EventDataDownloadStatus(const Download& download);

    GUID id;
    DownloadStatus status;
    UINT httpStatusCode;
};
