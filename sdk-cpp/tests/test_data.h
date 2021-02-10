#pragma once

#include <chrono>
#include <string>

extern const std::chrono::seconds g_docsIdleTimeoutSeconds;

extern const uint64_t g_smallFileSizeBytes;
extern const uint64_t g_largeFileSizeBytes;
extern const uint64_t g_prodFileSizeBytes;

extern const std::string g_docsProcName;
extern const std::string g_docsSvcName;
extern const std::string g_largeFileUrl;
extern const std::string g_malformedFilePath;
extern const std::string g_tmpFileName;
extern const std::string g_tmpFileName2;
extern const std::string g_tmpFileName3;
extern const std::string g_smallFileUrl;
extern const std::string g_404Url;
extern const std::string g_prodFileUrl; // Use this for MCC downloads. Other hostnames aren't enabled in MCC.

extern std::string g_mccHostName;
