// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _DELIVERY_OPTIMIZATION_TEST_DATA_H
#define _DELIVERY_OPTIMIZATION_TEST_DATA_H

#include <chrono>
#include <string>

#ifdef DO_BUILD_FOR_SNAP
// Using systemctl within snap package results in "Running in chroot, ignoring request".
// Use the snap command instead.
#define DO_SERVICE_CONTROLLER  "snap"
#else
#define DO_SERVICE_CONTROLLER  "systemctl"
#endif

extern const uint64_t g_smallFileSizeBytes;
extern const uint64_t g_largeFileSizeBytes;
extern const uint64_t g_prodFileSizeBytes;

#if defined(DO_INTERFACE_REST)
extern const std::string g_docsSvcName;
#elif defined(DO_INTERFACE_COM)
extern const std::string g_smallFilePhfInfoJson;
#endif

extern const std::string g_largeFileUrl;
extern const std::string g_malformedFilePath;
extern const std::string g_tmpFileName;
extern const std::string g_tmpFileName2;
extern const std::string g_tmpFileName3;
extern const std::string g_smallFileUrl;
extern const std::string g_smallFilePhfInfoJson;
extern const std::string g_404Url;
extern const std::string g_prodFileUrl; // Use this for MCC downloads. Other hostnames aren't enabled in MCC.

extern const std::chrono::seconds g_smallFileWaitTime;
extern const std::chrono::seconds g_largeFileWaitTime;

extern std::string g_mccHostName;
#endif
