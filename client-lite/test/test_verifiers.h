// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "do_test_helpers.h"
#include "download.h"
#include "download_manager.h"
#include "download_status.h"

inline void VerifyFileSize(const std::string& path, UINT64 cbExpectedSize)
{
    ASSERT_EQ(fs::file_size(path), cbExpectedSize) << "File size check: " << path;
}

inline void VerifyFileNotFound(const std::string& path)
{
    ASSERT_FALSE(fs::exists(path)) << "File absent check: " << path;
}

inline void VerifyNoError(const DownloadStatus& status)
{
    ASSERT_NE(status.State, DownloadState::Paused);
    ASSERT_EQ(status.Error, S_OK);
    ASSERT_EQ(status.ExtendedError, S_OK);
}

inline void VerifyError(const DownloadStatus& status, HRESULT expectedError, HRESULT expectedExtendedError = S_OK)
{
    ASSERT_EQ(status.State, DownloadState::Paused);
    ASSERT_EQ(status.Error, expectedError);
    ASSERT_EQ(status.ExtendedError, expectedExtendedError);
}

inline void VerifyState(const DownloadStatus& status, DownloadState expectedState)
{
    ASSERT_EQ(status.State, expectedState);
}

inline void VerifyDownloadNotFound(const DownloadManager& manager, const std::string& id)
{
    try
    {
        (void)manager.GetDownloadStatus(id);
        ASSERT_FALSE(true) << "Expected manager API call to fail";
    }
    catch (...)
    {
        auto hr = docli::ResultFromCaughtException();
        ASSERT_EQ(hr, E_NOT_SET);
    }
}

inline void VerifyDownloadComplete(const DownloadManager& manager, const std::string& id, UINT64 cbFile)
{
    auto status = manager.GetDownloadStatus(id);
    VerifyNoError(status);
    ASSERT_EQ(status.State, DownloadState::Transferred);
    ASSERT_EQ(status.BytesTotal, cbFile);
    ASSERT_EQ(status.BytesTransferred, cbFile);
}

inline void VerifyDownloadHttpStatus(const Download& download, UINT expectedHttpStatusCode)
{
    std::cout << "Response headers: " << download.ResponseHeaders() << std::endl;
    ASSERT_EQ(download.HttpStatusCode(), expectedHttpStatusCode);
    ASSERT_TRUE(!download.ResponseHeaders().empty());
}

inline void VerifyUrlAndPath(const DownloadManager& manager, const std::string& id, const std::string& url, const std::string& path)
{
    ASSERT_EQ(manager.GetDownloadProperty(id, DownloadProperty::Uri), url);
    ASSERT_EQ(manager.GetDownloadProperty(id, DownloadProperty::LocalPath), path);
}

inline void VerifyDOResultException(HRESULT hrExpected, const std::function<void()>& fnOperation)
{
    try
    {
        fnOperation();
        ASSERT_FALSE(true) << dotest::util::FormatString("Expected operation to generate exception with error 0x%x", hrExpected);
    }
    catch (const docli::DOResultException& ex)
    {
        ASSERT_EQ(ex.GetErrorCode(), hrExpected);
    }
}
