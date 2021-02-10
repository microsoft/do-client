
#include "do_common.h"
#include "event_data.h"

#include "download.h"

TelDataDownloadInfo::TelDataDownloadInfo(const Download& download) :
    _guid(download.GetId()),
    _status(download.GetStatus()),
    _url(download.GetUrl()),
    _destinationPath(download.GetDestinationPath()),
    _mccHost(download.GetMCCHost())
{
}

EventDataDownloadStarted::EventDataDownloadStarted(const Download& download) :
    _commonData(download)
{
}

EventDataDownloadCompleted::EventDataDownloadCompleted(const Download& download) :
    _commonData(download),
    _elapsedTime(download.GetElapsedTime())
{
}

EventDataDownloadPaused::EventDataDownloadPaused(const Download& download) :
    _commonData(download)
{
}

EventDataDownloadCanceled::EventDataDownloadCanceled(const Download& download) :
    _commonData(download)
{
}

EventDataDownloadStatus::EventDataDownloadStatus(const Download& download) :
    id(download.GetId()),
    status(download.Status()),
    httpStatusCode(download.HttpStatusCode())
{
}
