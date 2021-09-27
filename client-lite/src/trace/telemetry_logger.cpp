// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "do_common.h"
#include "telemetry_logger.h"

TelemetryLogger& TelemetryLogger::getInstance()
{
    static TelemetryLogger myInstance;
    return myInstance;
}

void TelemetryLogger::TraceDownloadStart(const EventDataDownloadStarted& eventData)
{
    DoLogInfoHr(eventData._commonData._status.Error, "id: %s, url: %s, filePath: %s, mccHost: %s",
        GuidToString(eventData._commonData._guid).data(), eventData._commonData._url.c_str(),
        eventData._commonData._destinationPath.c_str(), eventData._commonData._mccHost.c_str());
}

void TelemetryLogger::TraceDownloadCompleted(const EventDataDownloadCompleted& eventData)
{
    DoLogInfo("id: %s, url: %s, mccHost: %s, filePath: %s, bytes: [total: %ld, down: %ld], timeMS: %ld",
        GuidToString(eventData._commonData._guid).data(), eventData._commonData._url.c_str(), eventData._commonData._mccHost.c_str(),
        eventData._commonData._destinationPath.c_str(), eventData._commonData._status.BytesTotal,
        eventData._commonData._status.BytesTransferred, eventData._elapsedTime.count());
}

void TelemetryLogger::TraceDownloadPaused(const EventDataDownloadPaused& eventData)
{
    DoLogInfoHr(eventData._commonData._status.Error, "id: %s, extError: %x, cdnUrl: %s, mccHost: %s, filePath: %s, bytes: [total: %ld, down: %ld]",
        GuidToString(eventData._commonData._guid).data(), eventData._commonData._status.ExtendedError,  eventData._commonData._url.c_str(),
        eventData._commonData._mccHost.c_str(), eventData._commonData._destinationPath.c_str(), eventData._commonData._status.BytesTotal,
        eventData._commonData._status.BytesTransferred);
}

void TelemetryLogger::TraceDownloadCanceled(const EventDataDownloadCanceled& eventData)
{
    DoLogInfoHr(eventData._commonData._status.Error, "id: %s, extError: %x, cdnUrl: %s, mccHost: %s, filePath: %s, bytes: [total: %ld, down: %ld]",
        GuidToString(eventData._commonData._guid).data(), eventData._commonData._status.ExtendedError, eventData._commonData._url.c_str(),
        eventData._commonData._mccHost.c_str(), eventData._commonData._destinationPath.c_str(), eventData._commonData._status.BytesTotal,
        eventData._commonData._status.BytesTransferred);
}

void TelemetryLogger::TraceDownloadStatus(const EventDataDownloadStatus& eventData)
{
    DoLogVerbose("id: %s, %d, codes: [%u, 0x%x, 0x%x], %llu / %llu", GuidToString(eventData.id).c_str(), eventData.status.State,
        eventData.httpStatusCode, eventData.status.Error, eventData.status.ExtendedError, eventData.status.BytesTransferred,
        eventData.status.BytesTotal);
}
