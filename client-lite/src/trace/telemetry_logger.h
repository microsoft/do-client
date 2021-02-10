#pragma once

#include "do_noncopyable.h"
#include "event_data.h"

// TODO: Instrument telemetry provider
// Until telemetry is instrumented, this class serves as a wrapper for logging telemetry events with no data being sent
class TelemetryLogger : DONonCopyable
{
public:
    static TelemetryLogger& getInstance();

    void TraceDownloadStart(const EventDataDownloadStarted& eventData);
    void TraceDownloadCompleted(const EventDataDownloadCompleted& eventData);
    void TraceDownloadPaused(const EventDataDownloadPaused& eventData);
    void TraceDownloadCanceled(const EventDataDownloadCanceled& eventData);
    void TraceDownloadStatus(const EventDataDownloadStatus& eventData);
};