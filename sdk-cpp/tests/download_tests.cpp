#include "tests_common.h"

#include <atomic>
#include <string>
#include <thread>
#include <signal.h>
#include <sys/types.h>

#include <boost/filesystem.hpp>

#include "do_download.h"
#include "do_download_status.h"
#include "do_exceptions.h"
#include "do_test_helpers.h"
#include "test_data.h"
#include "test_helpers.h"

namespace msdo = microsoft::deliveryoptimization;
using namespace std::chrono_literals; // NOLINT(build/namespaces)

#define DO_ERROR_FROM_XPLAT_SYSERR(err) (int32_t)(0xC0000000 | (0xD0 << 16) | ((int32_t)(err) & 0x0000FFFF))
#define HTTP_E_STATUS_NOT_FOUND         ((int32_t)0x80190194L)

void WaitForDownloadCompletion(msdo::download& simpleDownload)
{
    msdo::download_status status = simpleDownload.get_status();
    const auto endtime = std::chrono::steady_clock::now() + 5min;
    while ((status.state() == msdo::download_state::transferring) && (std::chrono::steady_clock::now() < endtime))
    {
        std::this_thread::sleep_for(2s);
        status = simpleDownload.get_status();
        std::cout << "Transferred " << status.bytes_transferred() << " / " << status.bytes_total() << "\n";
    }

    ASSERT_EQ(status.state(), msdo::download_state::transferred) << "Download must have completed within timeout";
}

class DownloadTests : public ::testing::Test
{
public:
    void SetUp() override;
    void TearDown() override;
};

void DownloadTests::SetUp()
{
    TestHelpers::CleanTestDir();
}

void DownloadTests::TearDown()
{
    TestHelpers::CleanTestDir();
}

TEST_F(DownloadTests, SimpleDownloadTest)
{
    msdo::download simpleDownload(g_smallFileUrl, g_tmpFileName);
    msdo::download_status status = simpleDownload.get_status();
    ASSERT_EQ(status.state(), msdo::download_state::created);
    ASSERT_EQ(status.bytes_transferred(), 0u);

    simpleDownload.start();

    std::this_thread::sleep_for(5s);
    status = simpleDownload.get_status();
    ASSERT_EQ(status.state(), msdo::download_state::transferred);
    ASSERT_EQ(status.bytes_total(), status.bytes_transferred());
    ASSERT_EQ(status.bytes_total(), g_smallFileSizeBytes);

    simpleDownload.finalize();
    ASSERT_EQ(boost::filesystem::file_size(boost::filesystem::path(g_tmpFileName)), g_smallFileSizeBytes);
}

TEST_F(DownloadTests, SimpleBlockingDownloadTest)
{
    ASSERT_FALSE(boost::filesystem::exists(g_tmpFileName));
    msdo::download::download_url_to_path(g_smallFileUrl, g_tmpFileName);
    ASSERT_TRUE(boost::filesystem::exists(g_tmpFileName));
    ASSERT_EQ(boost::filesystem::file_size(boost::filesystem::path(g_tmpFileName)), g_smallFileSizeBytes);
}

TEST_F(DownloadTests, CancelBlockingDownloadTest)
{
    std::atomic_bool cancelToken { false };
    ASSERT_FALSE(boost::filesystem::exists(g_tmpFileName));
    std::thread downloadThread([&]()
    {
        try
        {
            msdo::download::download_url_to_path(g_largeFileUrl, g_tmpFileName, cancelToken);
            ASSERT_TRUE(false);
        }
        catch (const msdo::exception& e)
        {
            ASSERT_EQ(e.error_code(), static_cast<int32_t>(std::errc::operation_canceled));
        }
    });
    std::this_thread::sleep_for(1s);
    cancelToken = true;
    downloadThread.join();
    ASSERT_FALSE(boost::filesystem::exists(g_tmpFileName));
}

TEST_F(DownloadTests, BlockingDownloadTimeout)
{
    auto startTime = std::chrono::steady_clock::now();
    try
    {
        msdo::download::download_url_to_path(g_largeFileUrl, g_tmpFileName, std::chrono::seconds(2));
        ASSERT_TRUE(!"Expected download to time out");
    }
    catch (const msdo::exception& e)
    {
        ASSERT_EQ(e.error_code(), static_cast<int32_t>(std::errc::timed_out));
        auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - startTime);
        ASSERT_GE(elapsedTime, std::chrono::seconds(2));
        ASSERT_LE(elapsedTime, std::chrono::seconds(5));
    }
    ASSERT_FALSE(boost::filesystem::exists(g_tmpFileName));
}

// Note: This test takes a long time to execute due to 30 retry intervals from DOCS
TEST_F(DownloadTests, SimpleDownloadTest_With404Url)
{
    ASSERT_FALSE(boost::filesystem::exists(g_tmpFileName));

    try
    {
        msdo::download::download_url_to_path(g_404Url, g_tmpFileName);
        ASSERT_TRUE(false);
    }
    catch (const msdo::exception& e)
    {
        ASSERT_EQ(e.error_code(), HTTP_E_STATUS_NOT_FOUND);
    }
}

TEST_F(DownloadTests, SimpleDownloadTest_WithMalformedPath)
{
    try
    {
        msdo::download::download_url_to_path(g_smallFileUrl, g_malformedFilePath);
        ASSERT_TRUE(false);
    }
    catch (const msdo::exception& e)
    {
        ASSERT_EQ(e.error_code(), DO_ERROR_FROM_XPLAT_SYSERR(ENOENT));
        ASSERT_FALSE(boost::filesystem::exists(g_tmpFileName));
    }
}

TEST_F(DownloadTests, SimpleDownloadTest_With404UrlAndMalformedPath)
{
    ASSERT_FALSE(boost::filesystem::exists(g_tmpFileName));

    try
    {
        msdo::download::download_url_to_path(g_404Url, g_malformedFilePath);
        ASSERT_TRUE(false);
    }
    catch (const msdo::exception& e)
    {
        ASSERT_EQ(e.error_code(), DO_ERROR_FROM_XPLAT_SYSERR(ENOENT));
        ASSERT_FALSE(boost::filesystem::exists(g_tmpFileName));
    }
    catch (const std::exception& se)
    {
        std::cout << "Unexpected exception: " << se.what() << std::endl;
        ASSERT_TRUE(false);
    }
}

TEST_F(DownloadTests, Download1PausedDownload2SameDestTest)
{
    ASSERT_FALSE(boost::filesystem::exists(g_tmpFileName));
    msdo::download simpleDownload(g_largeFileUrl, g_tmpFileName);
    msdo::download_status status = simpleDownload.get_status();
    ASSERT_EQ(status.state(), msdo::download_state::created);
    ASSERT_EQ(status.bytes_transferred(), 0u);

    simpleDownload.start();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    simpleDownload.pause();
    status = simpleDownload.get_status();
    ASSERT_EQ(status.state(), msdo::download_state::paused);
    ASSERT_TRUE(boost::filesystem::exists(g_tmpFileName));

    msdo::download simpleDownload2(g_smallFileUrl, g_tmpFileName);
    try
    {
        simpleDownload2.start();
       ASSERT_TRUE(false);
    }
    catch (const msdo::exception& e)
    {
       ASSERT_EQ(e.error_code(), DO_ERROR_FROM_XPLAT_SYSERR(EEXIST));
    }
    simpleDownload2.abort();
    ASSERT_TRUE(boost::filesystem::exists(g_tmpFileName)); // not deleted, the earlier download is still active

    simpleDownload.abort();
    ASSERT_FALSE(boost::filesystem::exists(g_tmpFileName));

    // download2 should now succeed
    simpleDownload2 = msdo::download{g_smallFileUrl, g_tmpFileName};
    simpleDownload2.start();
    WaitForDownloadCompletion(simpleDownload2);
    ASSERT_EQ(boost::filesystem::file_size(g_tmpFileName), g_smallFileSizeBytes);
}

TEST_F(DownloadTests, Download1PausedDownload2SameFileDownload1Resume)
{
    ASSERT_FALSE(boost::filesystem::exists(g_tmpFileName));
    msdo::download simpleDownload(g_largeFileUrl, g_tmpFileName);
    msdo::download_status status = simpleDownload.get_status();
    ASSERT_EQ(status.state(), msdo::download_state::created);
    ASSERT_EQ(status.bytes_transferred(), 0u);

    simpleDownload.start();
    simpleDownload.pause();
    status = simpleDownload.get_status();
    ASSERT_EQ(status.state(), msdo::download_state::paused);

    msdo::download::download_url_to_path(g_largeFileUrl, g_tmpFileName2);
    ASSERT_EQ(boost::filesystem::file_size(boost::filesystem::path(g_tmpFileName2)), g_largeFileSizeBytes);

    simpleDownload.resume();
    WaitForDownloadCompletion(simpleDownload);
    ASSERT_EQ(boost::filesystem::file_size(boost::filesystem::path(g_tmpFileName)), g_largeFileSizeBytes);
}

TEST_F(DownloadTests, Download1NeverStartedDownload2CancelledSameFileTest)
{
    ASSERT_FALSE(boost::filesystem::exists(g_tmpFileName));
    msdo::download simpleDownload(g_largeFileUrl, g_tmpFileName);
    msdo::download_status status = simpleDownload.get_status();
    ASSERT_EQ(status.state(), msdo::download_state::created);
    ASSERT_EQ(status.bytes_transferred(), 0u);

    msdo::download simpleDownload2(g_largeFileUrl, g_tmpFileName);
    try
    {
        simpleDownload2.abort();
    }
    catch (const msdo::exception& e)
    {
        ASSERT_EQ(e.error_code(), static_cast<int32_t>(msdo::errc::not_found));
    }
    ASSERT_FALSE(boost::filesystem::exists(g_tmpFileName));
}

TEST_F(DownloadTests, ResumeOnAlreadyDownloadedFileTest)
{
    msdo::download simpleDownload(g_smallFileUrl, g_tmpFileName);
    msdo::download_status status = simpleDownload.get_status();
    ASSERT_EQ(status.state(), msdo::download_state::created);
    ASSERT_EQ(status.bytes_transferred(), 0u);

    simpleDownload.start();

    std::this_thread::sleep_for(5s);
    status = simpleDownload.get_status();
    ASSERT_EQ(status.state(), msdo::download_state::transferred);
    ASSERT_EQ(status.bytes_total(), status.bytes_transferred());
    ASSERT_EQ(status.bytes_total(), g_smallFileSizeBytes);

    simpleDownload.finalize();
    ASSERT_EQ(boost::filesystem::file_size(boost::filesystem::path(g_tmpFileName)), g_smallFileSizeBytes);

    try
    {
        simpleDownload.resume();
    }
    catch (const msdo::exception& e)
    {
        ASSERT_EQ(e.error_code(), static_cast<int32_t>(msdo::errc::not_found));
    }
}

TEST_F(DownloadTests, CancelDownloadOnCompletedState)
{
    msdo::download simpleDownload(g_smallFileUrl, g_tmpFileName);
    msdo::download_status status = simpleDownload.get_status();
    ASSERT_EQ(status.state(), msdo::download_state::created);
    ASSERT_EQ(status.bytes_transferred(), 0u);

    simpleDownload.start();

    std::this_thread::sleep_for(5s);
    status = simpleDownload.get_status();
    ASSERT_EQ(status.state(), msdo::download_state::transferred);
    ASSERT_EQ(status.bytes_total(), status.bytes_transferred());
    ASSERT_EQ(status.bytes_total(), g_smallFileSizeBytes);

    simpleDownload.finalize();
    ASSERT_EQ(boost::filesystem::file_size(boost::filesystem::path(g_tmpFileName)), g_smallFileSizeBytes);

    try
    {
        simpleDownload.abort();
    }
    catch (const msdo::exception& e)
    {
        ASSERT_EQ(e.error_code(), static_cast<int32_t>(msdo::errc::not_found));
    };
}

TEST_F(DownloadTests, CancelDownloadInTransferredState)
{
    msdo::download simpleDownload(g_smallFileUrl, g_tmpFileName);
    msdo::download_status status = simpleDownload.get_status();
    ASSERT_EQ(status.state(), msdo::download_state::created);
    ASSERT_EQ(status.bytes_transferred(), 0u);

    simpleDownload.start();

    std::this_thread::sleep_for(5s);
    status = simpleDownload.get_status();
    ASSERT_EQ(status.state(), msdo::download_state::transferred);
    ASSERT_EQ(status.bytes_total(), status.bytes_transferred());
    ASSERT_EQ(status.bytes_total(), g_smallFileSizeBytes);

    ASSERT_EQ(boost::filesystem::file_size(boost::filesystem::path(g_tmpFileName)), g_smallFileSizeBytes);
    try
    {
        simpleDownload.abort();
    }
    catch (const msdo::exception& e)
    {
        ASSERT_EQ(e.error_code(), static_cast<int32_t>(msdo::errc::not_found));
    }
}

static void _PauseResumeTest(bool delayAfterStart = false)
{
    msdo::download simpleDownload(g_largeFileUrl, g_tmpFileName);
    msdo::download_status status = simpleDownload.get_status();
    ASSERT_EQ(status.state(), msdo::download_state::created);
    ASSERT_EQ(status.bytes_transferred(), 0u);

    simpleDownload.start();
    if (delayAfterStart)
    {
        std::this_thread::sleep_for(1s);
    }
    simpleDownload.pause();
    status = simpleDownload.get_status();
    ASSERT_EQ(status.state(), msdo::download_state::paused);

    simpleDownload.resume();
    status = simpleDownload.get_status();
    ASSERT_EQ(status.state(), msdo::download_state::transferring);

    WaitForDownloadCompletion(simpleDownload);

    status = simpleDownload.get_status();
    ASSERT_EQ(status.state(), msdo::download_state::transferred);
    ASSERT_EQ(status.bytes_total(), status.bytes_transferred());
    simpleDownload.finalize();
    ASSERT_EQ(boost::filesystem::file_size(boost::filesystem::path(g_tmpFileName)), g_largeFileSizeBytes);
}

TEST_F(DownloadTests, PauseResumeTest)
{
    _PauseResumeTest();
}

TEST_F(DownloadTests, PauseResumeTestWithDelayAfterStart)
{
    _PauseResumeTest(true);
}

TEST_F(DownloadTests, MultipleConsecutiveDownloadTest)
{
    ASSERT_FALSE(boost::filesystem::exists(g_tmpFileName));
    msdo::download::download_url_to_path(g_smallFileUrl, g_tmpFileName);
    ASSERT_TRUE(boost::filesystem::exists(g_tmpFileName));
    ASSERT_EQ(boost::filesystem::file_size(boost::filesystem::path(g_tmpFileName)), g_smallFileSizeBytes);

    ASSERT_FALSE(boost::filesystem::exists(g_tmpFileName2));
    msdo::download::download_url_to_path(g_smallFileUrl, g_tmpFileName2);
    ASSERT_TRUE(boost::filesystem::exists(g_tmpFileName2));
    ASSERT_EQ(boost::filesystem::file_size(boost::filesystem::path(g_tmpFileName2)), g_smallFileSizeBytes);

    ASSERT_FALSE(boost::filesystem::exists(g_tmpFileName3));
    msdo::download::download_url_to_path(g_smallFileUrl, g_tmpFileName3);
    ASSERT_TRUE(boost::filesystem::exists(g_tmpFileName3));
    ASSERT_EQ(boost::filesystem::file_size(boost::filesystem::path(g_tmpFileName3)), g_smallFileSizeBytes);
}

TEST_F(DownloadTests, MultipleConcurrentDownloadTest)
{
    ASSERT_FALSE(boost::filesystem::exists(g_tmpFileName));
    std::thread downloadThread([&]()
    {
        try
        {
            msdo::download::download_url_to_path(g_smallFileUrl, g_tmpFileName);
        }
        catch (const msdo::exception& e)
        {
            ASSERT_TRUE(false);
        }
    });
    std::thread downloadThread2([&]()
    {
        try
        {
            msdo::download::download_url_to_path(g_smallFileUrl, g_tmpFileName2);
        }
        catch (const msdo::exception& e)
        {
            ASSERT_TRUE(false);
        }
    });
    std::thread downloadThread3([&]()
    {
        try
        {
            msdo::download::download_url_to_path(g_smallFileUrl, g_tmpFileName3);
        }
        catch (const msdo::exception& e)
        {
            ASSERT_TRUE(false);
        }
    });

    std::this_thread::sleep_for(3s);
    downloadThread.join();
    downloadThread2.join();
    downloadThread3.join();

    ASSERT_EQ(boost::filesystem::file_size(boost::filesystem::path(g_tmpFileName)), g_smallFileSizeBytes);
    ASSERT_EQ(boost::filesystem::file_size(boost::filesystem::path(g_tmpFileName2)), g_smallFileSizeBytes);
    ASSERT_EQ(boost::filesystem::file_size(boost::filesystem::path(g_tmpFileName3)), g_smallFileSizeBytes);
}

TEST_F(DownloadTests, MultipleConcurrentDownloadTest_WithCancels)
{
    std::atomic_bool cancelToken { false };

    std::thread downloadThread([&]()
    {
        try
        {
            msdo::download::download_url_to_path(g_smallFileUrl, g_tmpFileName);
        }
        catch (const msdo::exception& e)
        {
            ASSERT_TRUE(false);
        }
    });
    std::thread downloadThread2([&]()
    {
        try
        {
            msdo::download::download_url_to_path(g_largeFileUrl, g_tmpFileName2, cancelToken);
            ASSERT_TRUE(false); // Cancel will cause download_url_to_path to throw, so reaching here would be unexpected.
        }
        catch (const msdo::exception& e)
        {
            ASSERT_EQ(e.error_code(), static_cast<int32_t>(std::errc::operation_canceled));
        }
    });
    std::thread downloadThread3([&]()
    {
        try
        {
            msdo::download::download_url_to_path(g_smallFileUrl, g_tmpFileName3);
        }
        catch (const msdo::exception& e)
        {
            ASSERT_TRUE(false);
        }
    });

    std::this_thread::sleep_for(2s);
    cancelToken = true;

    downloadThread.join();
    downloadThread2.join();
    downloadThread3.join();

    ASSERT_EQ(boost::filesystem::file_size(boost::filesystem::path(g_tmpFileName)), g_smallFileSizeBytes);
    ASSERT_FALSE(boost::filesystem::exists(g_tmpFileName2));
    ASSERT_EQ(boost::filesystem::file_size(boost::filesystem::path(g_tmpFileName3)), g_smallFileSizeBytes);
}

TEST_F(DownloadTests, SimpleBlockingDownloadTest_ClientNotRunning)
{
    TestHelpers::StopService("deliveryoptimization-agent.service");
    auto startService = dotest::util::scope_exit([]()
        {
            TestHelpers::StartService("deliveryoptimization-agent.service");
        });
    TestHelpers::DeleteRestPortFiles(); // can be removed if docs deletes file on shutdown

    ASSERT_FALSE(boost::filesystem::exists(g_tmpFileName));
    try
    {
        msdo::download::download_url_to_path(g_smallFileUrl, g_tmpFileName);
        ASSERT_TRUE(!"Expected operation to throw exception");
    }
    catch (const msdo::exception& ex)
    {
        ASSERT_EQ(ex.error_code(), static_cast<int32_t>(msdo::errc::no_service));
    }
    ASSERT_FALSE(boost::filesystem::exists(g_tmpFileName));
}

TEST_F(DownloadTests, SimpleBlockingDownloadTest_ClientNotRunningPortFilePresent)
{
    // TODO(shishirb) Service name should come from cmake
    TestHelpers::StopService("deliveryoptimization-agent.service");
    auto startService = dotest::util::scope_exit([]()
        {
            TestHelpers::StartService("deliveryoptimization-agent.service");
        });
    TestHelpers::DeleteRestPortFiles();
    TestHelpers::CreateRestPortFiles(1);

    ASSERT_FALSE(boost::filesystem::exists(g_tmpFileName));
    try
    {
        msdo::download::download_url_to_path(g_smallFileUrl, g_tmpFileName);
        ASSERT_TRUE(!"Expected operation to throw exception");
    }
    catch (const msdo::exception& ex)
    {
        ASSERT_EQ(ex.error_code(), static_cast<int32_t>(msdo::errc::no_service));
    }
    ASSERT_FALSE(boost::filesystem::exists(g_tmpFileName));
}


TEST_F(DownloadTests, MultipleRestPortFileExists_Download)
{
// Enable after we have the ability to start the daemon after creating rest port files.
// This will ensure that the actual rest port file will have the latest timestamp.
#if 0
    TestHelpers::CreateRestPortFiles(5);

    ASSERT_FALSE(boost::filesystem::exists(g_tmpFileName));
    msdo::download::download_url_to_path(g_smallFileUrl, g_tmpFileName);
    ASSERT_TRUE(boost::filesystem::exists(g_tmpFileName));
    ASSERT_EQ(boost::filesystem::file_size(boost::filesystem::path(g_tmpFileName)), g_smallFileSizeBytes);

    TestHelpers::CleanupWorkingDir();
#endif
}
