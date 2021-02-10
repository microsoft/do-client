#include "test_common.h"

#include "config_manager.h"
#include "do_error.h"
#include "download.h"
#include "download_manager.h"
#include "test_data.h"
#include "test_verifiers.h"

using namespace std::chrono_literals; // NOLINT(build/namespaces)

static HRESULT StartAndWaitUntilNotTransferring(const DownloadManager& manager, const std::string& id,
    std::chrono::milliseconds waitTime = 5s) try
{
    manager.StartDownload(id);

    auto endTime = std::chrono::steady_clock::now() + waitTime;
    while ((std::chrono::steady_clock::now() < endTime) &&
        (manager.GetDownloadStatus(id).State == DownloadState::Transferring))
    {
        std::this_thread::sleep_for(1s);
    }

    return S_OK;

} CATCH_RETURN()

std::shared_ptr<Download> DownloadForId(const DownloadManager& manager, const std::string& id)
{
    return manager._GetDownload(id);
}

class DownloadManagerTests : public ::testing::Test
{
protected:
    ConfigManager configs;
    DownloadManager manager { configs };
};

TEST_F(DownloadManagerTests, EmptyDownload)
{
    const auto id = manager.CreateDownload();
    const std::string downloadId = manager.GetDownloadProperty(id, DownloadProperty::Id);
    ASSERT_NE(downloadId, std::string{});
    ASSERT_TRUE(StringToGuid(downloadId.data()));

    ASSERT_EQ(manager.GetDownloadProperty(id, DownloadProperty::Uri), std::string{});
    ASSERT_EQ(manager.GetDownloadProperty(id, DownloadProperty::LocalPath), std::string{});

    auto status = manager.GetDownloadStatus(id);
    ASSERT_EQ(status.BytesTotal, 0);
    ASSERT_EQ(status.BytesTransferred, 0);
    ASSERT_EQ(status.State, DownloadState::Created);
    VerifyNoError(status);

    auto emptyDownload = DownloadForId(manager, id);
    ASSERT_EQ(emptyDownload->HttpStatusCode(), 0);
    ASSERT_EQ(emptyDownload->ResponseHeaders(), std::string{});
    manager.AbortDownload(id);
    ASSERT_EQ(emptyDownload->GetStatus().State, DownloadState::Aborted);
}

TEST_F(DownloadManagerTests, SmallFileDownload)
{
    ClearTempDir();

    const std::string destFile = g_testTempDir / "smallfile.test";
    const std::string id = manager.CreateDownload(g_smallFileUrl, destFile);
    VerifyUrlAndPath(manager, id, g_smallFileUrl, destFile);
    ASSERT_EQ(StartAndWaitUntilNotTransferring(manager, id), S_OK);

    VerifyDownloadComplete(manager, id, 1837);
    VerifyDownloadHttpStatus(*DownloadForId(manager, id), HTTP_STATUS_OK);
    manager.FinalizeDownload(id);
    VerifyDownloadNotFound(manager, id);
}

TEST_F(DownloadManagerTests, MultipleDownloads)
{
    ClearTempDir();

    const std::string destFile1 = g_testTempDir / "smallfile.test";
    const std::string id1 = manager.CreateDownload(g_smallFileUrl, destFile1);
    VerifyUrlAndPath(manager, id1, g_smallFileUrl, destFile1);
    ASSERT_EQ(StartAndWaitUntilNotTransferring(manager, id1), S_OK);

    const std::string destFile2 = g_testTempDir / "nonexistentfile.test";
    const std::string id2 = manager.CreateDownload(g_404Url, destFile2);

    const std::string destFile3 = g_testTempDir / "largefile.test";
    const std::string id3 = manager.CreateDownload();
    manager.SetDownloadProperty(id3, DownloadProperty::Uri, g_largeFileUrl);
    manager.SetDownloadProperty(id3, DownloadProperty::LocalPath, destFile3);
    VerifyUrlAndPath(manager, id3, g_largeFileUrl, destFile3);

    manager.StartDownload(id2);
    manager.StartDownload(id3);

    VerifyDownloadComplete(manager, id1, 1837);
    manager.FinalizeDownload(id1);
    VerifyDownloadNotFound(manager, id1);

    std::this_thread::sleep_for(2s);
    auto status2 = manager.GetDownloadStatus(id2);
    VerifyError(status2, HTTP_E_STATUS_NOT_FOUND);

    auto status3 = manager.GetDownloadStatus(id3);
    ASSERT_TRUE((status3.State == DownloadState::Transferring) || (status3.State == DownloadState::Transferred))
        << "State: " << static_cast<int>(status3.State);

    manager.AbortDownload(id2);
    VerifyDownloadNotFound(manager, id2);
    manager.AbortDownload(id3);
    VerifyDownloadNotFound(manager, id3);
    VerifyFileNotFound(destFile2);
    VerifyFileNotFound(destFile3);
    VerifyFileSize(destFile1, 1837);
}

TEST_F(DownloadManagerTests, FileDownloadFatal404)
{
    const std::string destFile = g_testTempDir / "nonexistentfile.test";
    const std::string id = manager.CreateDownload(g_404Url, destFile);
    VerifyUrlAndPath(manager, id, g_404Url, destFile);

    // Expect immediate failure upon starting
    manager.StartDownload(id);
    std::this_thread::sleep_for(2s);
    const auto status = manager.GetDownloadStatus(id);
    ASSERT_EQ(status.BytesTransferred, 0);
    ASSERT_EQ(status.BytesTotal, 0);
    VerifyError(status, HTTP_E_STATUS_NOT_FOUND);
    VerifyDownloadHttpStatus(*DownloadForId(manager, id), 404);
    ASSERT_TRUE(cppfs::exists(destFile));
    manager.AbortDownload(id);
    VerifyFileNotFound(destFile);
}

TEST_F(DownloadManagerTests, FileMidDownloadTransient404)
{
    ClearTempDir();

    const std::string destFile = g_testTempDir / "largefile.test";
    const std::string id = manager.CreateDownload(g_largeFileUrl, destFile);
    manager.StartDownload(id);
    std::this_thread::sleep_for(2s);
    manager.PauseDownload(id);
    const auto status = manager.GetDownloadStatus(id);
    ASSERT_EQ(status.State, DownloadState::Paused);
    ASSERT_NE(status.BytesTransferred, 0);

    // Change to 404 URL and expect immediate failure
    manager.SetDownloadProperty(id, DownloadProperty::Uri, g_404Url);
    manager.SetDownloadProperty(id, DownloadProperty::NoProgressTimeoutSeconds, "22");
    manager.StartDownload(id);
    std::this_thread::sleep_for(2s);
    const auto status2 = manager.GetDownloadStatus(id);
    ASSERT_EQ(status.BytesTransferred, status2.BytesTransferred);
    VerifyError(status2, HTTP_E_STATUS_NOT_FOUND);

    // Switch back to valid URL and wait for completion
    manager.SetDownloadProperty(id, DownloadProperty::Uri, g_largeFileUrl);
    ASSERT_EQ(StartAndWaitUntilNotTransferring(manager, id, 5min), S_OK);
    VerifyDownloadComplete(manager, id, 536870440);
    ASSERT_EQ(manager.GetDownloadProperty(id, DownloadProperty::NoProgressTimeoutSeconds), "22");
    manager.FinalizeDownload(id);
    VerifyFileSize(destFile, 536870440);
    VerifyDownloadNotFound(manager, id);
}

TEST_F(DownloadManagerTests, HttpsFileDownload)
{
    ClearTempDir();

    const std::string destFile = g_testTempDir / "httpsfile.test";
    const std::string url = "https://omextemplates.content.office.net/support/templates/en-us/tf01228997.accdt";
    const std::string id = manager.CreateDownload(url, destFile);
    VerifyUrlAndPath(manager, id, url, destFile);
    ASSERT_EQ(StartAndWaitUntilNotTransferring(manager, id), S_OK);
    VerifyDownloadComplete(manager, id, 2054738);
    manager.FinalizeDownload(id);
    VerifyFileSize(destFile, 2054738);
}

TEST_F(DownloadManagerTests, PauseResumeDownload)
{
    ClearTempDir();

    const std::string destFile = g_testTempDir / "largefile.test";

    const auto id = manager.CreateDownload(g_largeFileUrl, destFile);
    manager.StartDownload(id);
    std::this_thread::sleep_for(1s);
    VerifyNoError(manager.GetDownloadStatus(id));
    manager.PauseDownload(id);

    auto status = manager.GetDownloadStatus(id);
    ASSERT_EQ(status.State, DownloadState::Paused);
    ASSERT_EQ(status.Error, S_OK);
    ASSERT_EQ(status.ExtendedError, S_OK);
    ASSERT_EQ(status.BytesTotal, 536870440);
    ASSERT_NE(status.BytesTransferred, 0);
    VerifyFileSize(destFile, status.BytesTransferred);

    ASSERT_EQ(StartAndWaitUntilNotTransferring(manager, id, 5min), S_OK);
    VerifyDownloadComplete(manager, id, 536870440);
    VerifyDownloadHttpStatus(*DownloadForId(manager, id), HTTP_STATUS_PARTIAL_CONTENT);
    manager.FinalizeDownload(id);
    VerifyFileSize(destFile, 536870440);
}

// Test fails when using cpprestsdk built without this change (automatic follow of redirects)
// made on Mar 31, 2020 - https://github.com/microsoft/cpprestsdk/pull/1328
TEST_F(DownloadManagerTests, DownloadUrlWithRedirects)
{
    const std::string destFile = g_testTempDir / "redirect.test";
    const std::string url = "http://raspbian.raspberrypi.org/raspbian/pool/main/b/boost1.62/libboost-random1.62.0_1.62.0+dfsg-4_armhf.deb";
    const auto id = manager.CreateDownload(url, destFile);
    ASSERT_EQ(StartAndWaitUntilNotTransferring(manager, id, std::chrono::seconds(30)), S_OK);

    VerifyDownloadComplete(manager, id, 34836);
    VerifyDownloadHttpStatus(*DownloadForId(manager, id), HTTP_STATUS_OK);
}

TEST_F(DownloadManagerTests, InvalidUrl)
{
    const auto id = manager.CreateDownload();
    VerifyDOResultException(INET_E_INVALID_URL, [&]()
    {
        manager.SetDownloadProperty(id, DownloadProperty::Uri, "invaliduri:http//");
    });
    VerifyDOResultException(INET_E_INVALID_URL, [&]()
    {
        manager.SetDownloadProperty(id, DownloadProperty::Uri, "invalidurihttp");
    });
    VerifyDOResultException(INET_E_INVALID_URL, [&]()
    {
        manager.SetDownloadProperty(id, DownloadProperty::Uri, "http://");
    });
    VerifyDOResultException(INET_E_INVALID_URL, [&]()
    {
        manager.SetDownloadProperty(id, DownloadProperty::Uri, "ftp://server.com");
    });

    VerifyDOResultException(INET_E_INVALID_URL, [&]()
    {
        manager.CreateDownload("invaliduri:http//");
    });
}

TEST_F(DownloadManagerTests, InvalidState)
{
    const std::string destFile = g_testTempDir / "largefile.test";
    const auto id = manager.CreateDownload(g_largeFileUrl);
    manager.SetDownloadProperty(id, DownloadProperty::LocalPath, destFile);
    manager.StartDownload(id);
    std::this_thread::sleep_for(1s);
    VerifyState(manager.GetDownloadStatus(id), DownloadState::Transferring);
    VerifyDOResultException(DO_E_INVALID_STATE, [&]()
    {
        manager.SetDownloadProperty(id, DownloadProperty::Uri, "http://www.example.com");
    });
    VerifyDOResultException(DO_E_INVALID_STATE, [&]()
    {
        manager.SetDownloadProperty(id, DownloadProperty::LocalPath, destFile);
    });
    manager.PauseDownload(id);
    manager.SetDownloadProperty(id, DownloadProperty::Uri, "http://www.example.com"); // should succeed now
    VerifyDOResultException(DO_E_INVALID_STATE, [&]()
    {
        manager.SetDownloadProperty(id, DownloadProperty::LocalPath, destFile);
    });
}
