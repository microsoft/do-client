#pragma once

#include <chrono>

constexpr auto g_mccHostBanInterval = std::chrono::minutes(5);
constexpr auto g_progressTrackerCheckInterval = std::chrono::seconds(10);
constexpr UINT g_progressTrackerMaxNoProgressIntervals = 30;
constexpr UINT g_progressTrackerMaxNoProgressIntervalsWithMCC = 6;
constexpr auto g_progressTrackerMaxRetryDelay = std::chrono::seconds(30);

// Provides a wait time of ~49 days which should be sufficient for uses
// of steady_clock (timing, event waits, condition_variable waits).
constexpr auto g_steadyClockInfiniteWaitTime = std::chrono::milliseconds(MAXUINT);

#define DEF_TRACE_FILE_MAXSIZE_BYTES    (8U * 1024 * 1024)
#define DEF_TRACE_FOLDER_MAXSIZE_BYTES  (DEF_TRACE_FILE_MAXSIZE_BYTES * 2)
#define DEF_TRACE_LEVEL                 boost::log::trivial::trace

const char* const ConfigName_AduIoTConnectionString = "ADUC_IoTConnectionString";

const char* const ConfigName_CacheHostFallbackDelayBgSecs = "DODelayCacheServerFallbackBackground";
const char* const ConfigName_CacheHostFallbackDelayFgSecs = "DODelayCacheServerFallbackForeground";
constexpr auto g_cacheHostFallbackDelayDefault = std::chrono::seconds(0);       // default: immediate fallback
constexpr auto g_cacheHostFallbackDelayNoFallback = std::chrono::seconds(-1);   // fallback to CDN not allowed

const char* const ConfigName_CacheHostServer = "DOCacheHost";

const char* const ConfigName_RestControllerValidateRemoteAddr = "RestControllerValidateRemoteAddr";
constexpr auto g_RestControllerValidateRemoteAddrDefault = true; // default: enabled
