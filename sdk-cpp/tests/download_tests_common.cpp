// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "tests_common.h"

#include <atomic>
#include <array>
#include <iostream>
#include <string>
#include <thread>
#include <signal.h>
#include <sys/types.h>

#include <boost/filesystem.hpp>

#include "do_download.h"
#include "do_download_status.h"
#include "do_errors.h"
#include "do_test_helpers.h"
#include "test_data.h"
#include "test_helpers.h"

namespace msdo = microsoft::deliveryoptimization;
namespace msdod = microsoft::deliveryoptimization::details;
namespace msdot = microsoft::deliveryoptimization::test;
using namespace std::chrono_literals; // NOLINT(build/namespaces)

#ifndef HTTP_E_STATUS_NOT_FOUND
#define HTTP_E_STATUS_NOT_FOUND         ((int)0x80190194L)
#endif

#ifndef E_ACCESSDENIED
#define E_ACCESSDENIED      ((int)0x80070005)
#endif

// Enables verifying of some errors returned from DO agent
#ifndef DO_ERROR_FROM_SYSTEM_ERROR
#define DO_ERROR_FROM_SYSTEM_ERROR(x) (int32_t)(0xC0000000 | (FACILITY_DELIVERY_OPTIMIZATION << 16) | ((int32_t)(x) & 0x0000FFFF))
#endif

static void WaitForDownloadCompletion(msdot::download& simpleDownload)
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
    void SetUp() override
    {
        TestHelpers::CleanTestDir();
    }
    void TearDown() override
    {
        TestHelpers::CleanTestDir();
    }
};

TEST_F(DownloadTests, SimpleDownloadTest)
{
    auto simpleDownload = msdot::download::make(g_smallFileUrl, g_tmpFileName);
    msdo::download_status status = simpleDownload->get_status();
    ASSERT_EQ(status.state(), msdo::download_state::created);
    ASSERT_EQ(status.bytes_transferred(), 0u);

    simpleDownload->start();

    TestHelpers::WaitForState(*simpleDownload, msdo::download_state::transferred, g_smallFileWaitTime);
    status = simpleDownload->get_status();
    ASSERT_EQ(status.bytes_total(), status.bytes_transferred());
    ASSERT_EQ(status.bytes_total(), g_smallFileSizeBytes);

    simpleDownload->finalize();
    ASSERT_EQ(boost::filesystem::file_size(boost::filesystem::path(g_tmpFileName)), g_smallFileSizeBytes);
}

TEST_F(DownloadTests, SimpleBlockingDownloadTest)
{
    ASSERT_FALSE(boost::filesystem::exists(g_tmpFileName));
    msdot::download::download_url_to_path(g_smallFileUrl, g_tmpFileName);
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
            msdot::download::download_url_to_path(g_largeFileUrl, g_tmpFileName, cancelToken);
            ASSERT_TRUE(false);
        }
        catch (const msdod::exception& e)
        {
            ASSERT_EQ(e.error_code().value(), static_cast<int>(std::errc::operation_canceled));
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
        msdot::download::download_url_to_path(g_largeFileUrl, g_tmpFileName, std::chrono::seconds(2));
        ASSERT_TRUE(!"Expected download to time out");
    }
    catch (const msdod::exception& e)
    {
        ASSERT_EQ(e.error_code().value(), static_cast<int>(std::errc::timed_out));
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
        msdot::download::download_url_to_path(g_404Url, g_tmpFileName);
        ASSERT_TRUE(false);
    }
    catch (const msdod::exception& e)
    {
        ASSERT_EQ(e.error_code().value(), HTTP_E_STATUS_NOT_FOUND);
    }
}

TEST_F(DownloadTests, SimpleDownloadTest_WithMalformedPath)
{
    try
    {
        msdot::download::download_url_to_path(g_smallFileUrl, g_malformedFilePath);
        ASSERT_TRUE(false);
    }
    catch (const msdod::exception& e)
    {
#if defined(DO_INTERFACE_COM)
        constexpr auto c_invalidDeviceName = static_cast<int>(0x8007007b);
        std::array<int, 3> expectedErrors{ E_ACCESSDENIED, HTTP_E_STATUS_NOT_FOUND, c_invalidDeviceName };
        // DO returns different errors on dev machine and pipeline agents (Win10/Win11?)
        ASSERT_TRUE(std::find(expectedErrors.begin(), expectedErrors.end(), e.error_code().value()) != expectedErrors.end())
            << e.error_code().value();
#elif defined(DO_INTERFACE_REST)
        ASSERT_EQ(e.error_code().value(), DO_ERROR_FROM_SYSTEM_ERROR(ENOENT));
#endif
        ASSERT_FALSE(boost::filesystem::exists(g_tmpFileName));
    }
}

TEST_F(DownloadTests, SimpleDownloadTest_With404UrlAndMalformedPath)
{
    ASSERT_FALSE(boost::filesystem::exists(g_tmpFileName));

    try
    {
        msdot::download::download_url_to_path(g_404Url, g_malformedFilePath);
        ASSERT_TRUE(false);
    }
    catch (const msdod::exception& e)
    {
#if defined(DO_INTERFACE_COM)
        constexpr auto c_invalidDeviceName = static_cast<int>(0x8007007b);
        std::array<int, 3> expectedErrors{ E_ACCESSDENIED, HTTP_E_STATUS_NOT_FOUND, c_invalidDeviceName };
        // DO returns different errors on dev machine and pipeline agents (Win10/Win11?)
        ASSERT_TRUE(std::find(expectedErrors.begin(), expectedErrors.end(), e.error_code().value()) != expectedErrors.end())
            << e.error_code().value();
#elif defined(DO_INTERFACE_REST)
        ASSERT_EQ(e.error_code().value(), DO_ERROR_FROM_SYSTEM_ERROR(ENOENT));
#endif
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
    auto simpleDownload = msdot::download::make(g_largeFileUrl, g_tmpFileName);
    msdo::download_status status = simpleDownload->get_status();
    ASSERT_EQ(status.state(), msdo::download_state::created);
    ASSERT_EQ(status.bytes_transferred(), 0u);

    simpleDownload->start();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    simpleDownload->pause();
    status = simpleDownload->get_status();
    ASSERT_EQ(status.state(), msdo::download_state::paused);

#ifdef DO_CLIENT_AGENT
    // Only DO Agent creates the output file upon calling start()
    ASSERT_TRUE(boost::filesystem::exists(g_tmpFileName)) << "Verify output file created for first download";

    // DO Agent does not support downloading to an already existing path (yet?).
    // Verify second download fails and the first download resumes successfully.
    auto simpleDownload2 = msdot::download::make(g_smallFileUrl, g_tmpFileName);
    try
    {
        simpleDownload2->start();
        ASSERT_TRUE(false);
    }
    catch (const msdod::exception& e)
    {
       ASSERT_EQ(e.error_code().value(), DO_ERROR_FROM_SYSTEM_ERROR(EEXIST));
    }
    simpleDownload2->abort();
    ASSERT_TRUE(boost::filesystem::exists(g_tmpFileName)); // not deleted, the earlier download is still active

    simpleDownload->abort();
    ASSERT_FALSE(boost::filesystem::exists(g_tmpFileName));

    // download2 should now succeed
    simpleDownload2 = msdot::download::make(g_smallFileUrl, g_tmpFileName);
    simpleDownload2->start();
    WaitForDownloadCompletion(*simpleDownload2);
    ASSERT_EQ(boost::filesystem::file_size(g_tmpFileName), g_smallFileSizeBytes);

#elif defined(DO_CLIENT_DOSVC)
    // DoSvc does support downloading to an already existing path via
    // aborting the first download. Verify this and that the second download succeeds.

    msdo::test::download::download_url_to_path(g_smallFileUrl, g_tmpFileName);
    ASSERT_EQ(boost::filesystem::file_size(boost::filesystem::path(g_tmpFileName)), g_smallFileSizeBytes);

    status = simpleDownload->get_status();
    ASSERT_EQ(status.state(), msdo::download_state::aborted);

#else
#error "Target client undefined"
#endif
}

TEST_F(DownloadTests, Download1PausedDownload2SameFileDownload1Resume)
{
    ASSERT_FALSE(boost::filesystem::exists(g_tmpFileName));
    auto simpleDownload = msdot::download::make(g_largeFileUrl, g_tmpFileName);
    msdo::download_status status = simpleDownload->get_status();
    ASSERT_EQ(status.state(), msdo::download_state::created);
    ASSERT_EQ(status.bytes_transferred(), 0u);

    simpleDownload->start();
    std::this_thread::sleep_for(3s);
    simpleDownload->pause();
    std::cout << "Waiting for state to change to paused" << std::endl;
    TestHelpers::WaitForState(*simpleDownload, msdo::download_state::paused);

    std::cout << "Downloading the same file with a second download" << std::endl;
    msdot::download::download_url_to_path(g_largeFileUrl, g_tmpFileName2);
    ASSERT_EQ(boost::filesystem::file_size(boost::filesystem::path(g_tmpFileName2)), g_largeFileSizeBytes);

    std::cout << "Resuming and waiting for completion of first download" << std::endl;
    simpleDownload->resume();
    TestHelpers::WaitForState(*simpleDownload, msdo::download_state::transferred, g_largeFileWaitTime);

    simpleDownload->finalize();
    ASSERT_EQ(boost::filesystem::file_size(boost::filesystem::path(g_tmpFileName)), g_largeFileSizeBytes);
}

TEST_F(DownloadTests, Download1NeverStartedDownload2CancelledSameFileTest)
{
    ASSERT_FALSE(boost::filesystem::exists(g_tmpFileName));
    auto simpleDownload = msdot::download::make(g_largeFileUrl, g_tmpFileName);
    msdo::download_status status = simpleDownload->get_status();
    ASSERT_EQ(status.state(), msdo::download_state::created);
    ASSERT_EQ(status.bytes_transferred(), 0u);

    auto simpleDownload2 = msdot::download::make(g_largeFileUrl, g_tmpFileName);
    try
    {
        simpleDownload2->abort();
    }
    catch (const msdod::exception& e)
    {
        ASSERT_EQ(e.error_code().value(), static_cast<int>(msdo::errc::not_found));
    }
    ASSERT_FALSE(boost::filesystem::exists(g_tmpFileName));
}

TEST_F(DownloadTests, ResumeOnAlreadyDownloadedFileTest)
{
    auto simpleDownload = msdot::download::make(g_smallFileUrl, g_tmpFileName);
    msdo::download_status status = simpleDownload->get_status();
    ASSERT_EQ(status.state(), msdo::download_state::created);
    ASSERT_EQ(status.bytes_transferred(), 0u);

    simpleDownload->start();

    TestHelpers::WaitForState(*simpleDownload, msdo::download_state::transferred, g_smallFileWaitTime);
    status = simpleDownload->get_status();
    ASSERT_EQ(status.bytes_total(), status.bytes_transferred());
    ASSERT_EQ(status.bytes_total(), g_smallFileSizeBytes);

    simpleDownload->finalize();
    ASSERT_EQ(boost::filesystem::file_size(boost::filesystem::path(g_tmpFileName)), g_smallFileSizeBytes);

    try
    {
        simpleDownload->resume();
    }
    catch (const msdod::exception& e)
    {
#if defined(DO_INTERFACE_COM)
        ASSERT_EQ(e.error_code().value(), static_cast<int>(msdo::errc::do_e_invalid_state));
#elif defined(DO_INTERFACE_REST)
        ASSERT_EQ(e.error_code().value(), static_cast<int>(msdo::errc::not_found));
#endif
    }
}

TEST_F(DownloadTests, CancelDownloadOnCompletedState)
{
    auto simpleDownload = msdot::download::make(g_smallFileUrl, g_tmpFileName);
    msdo::download_status status = simpleDownload->get_status();
    ASSERT_EQ(status.state(), msdo::download_state::created);
    ASSERT_EQ(status.bytes_transferred(), 0u);

    simpleDownload->start();

    std::this_thread::sleep_for(5s);
    status = simpleDownload->get_status();
    ASSERT_EQ(status.state(), msdo::download_state::transferred);
    ASSERT_EQ(status.bytes_total(), status.bytes_transferred());
    ASSERT_EQ(status.bytes_total(), g_smallFileSizeBytes);

    simpleDownload->finalize();
    ASSERT_EQ(boost::filesystem::file_size(boost::filesystem::path(g_tmpFileName)), g_smallFileSizeBytes);

    try
    {
        simpleDownload->abort();
    }
    catch (const msdod::exception& e)
    {
#if defined(DO_INTERFACE_COM)
        ASSERT_EQ(e.error_code().value(), static_cast<int>(msdo::errc::do_e_invalid_state));
#elif defined(DO_INTERFACE_REST)
        ASSERT_EQ(e.error_code().value(), static_cast<int>(msdo::errc::not_found));
#endif
    };
}

TEST_F(DownloadTests, CancelDownloadInTransferredState)
{
    auto simpleDownload = msdot::download::make(g_smallFileUrl, g_tmpFileName);
    msdo::download_status status = simpleDownload->get_status();
    ASSERT_EQ(status.state(), msdo::download_state::created);
    ASSERT_EQ(status.bytes_transferred(), 0u);

    simpleDownload->start();

    TestHelpers::WaitForState(*simpleDownload, msdo::download_state::transferred, g_smallFileWaitTime);
    status = simpleDownload->get_status();
    ASSERT_EQ(status.bytes_total(), status.bytes_transferred());
    ASSERT_EQ(status.bytes_total(), g_smallFileSizeBytes);

#if defined(DO_INTERFACE_REST)
    // On Windows need to finalize in order for file to show up on disk, so exclude check here
    ASSERT_EQ(boost::filesystem::file_size(boost::filesystem::path(g_tmpFileName)), g_smallFileSizeBytes);
#endif
    try
    {
        simpleDownload->abort();
    }
    catch (const msdod::exception& e)
    {
#if defined(DO_INTERFACE_COM)
        ASSERT_EQ(e.error_code().value(), static_cast<int>(msdo::errc::do_e_invalid_state));
#elif defined(DO_INTERFACE_REST)
        ASSERT_EQ(e.error_code().value(), static_cast<int>(msdo::errc::not_found));
#endif
    }
}

static void _PauseResumeTest(bool delayAfterStart = false)
{
    auto simpleDownload = msdot::download::make(g_largeFileUrl, g_tmpFileName);
    msdo::download_status status = simpleDownload->get_status();
    ASSERT_EQ(status.state(), msdo::download_state::created);
    ASSERT_EQ(status.bytes_transferred(), 0u);

    simpleDownload->start();
    if (delayAfterStart)
    {
#if defined(DO_INTERFACE_REST)
        std::this_thread::sleep_for(1s);
#elif defined(DO_INTERFACE_COM)
        std::this_thread::sleep_for(5s);
#endif
    }
    simpleDownload->pause();
    TestHelpers::WaitForState(*simpleDownload, msdo::download_state::paused);

    simpleDownload->resume();
    TestHelpers::WaitForState(*simpleDownload, msdo::download_state::transferred, g_largeFileWaitTime);

    status = simpleDownload->get_status();
    ASSERT_EQ(status.state(), msdo::download_state::transferred);
    ASSERT_EQ(status.bytes_total(), status.bytes_transferred());
    simpleDownload->finalize();
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
    msdot::download::download_url_to_path(g_smallFileUrl, g_tmpFileName);
    ASSERT_TRUE(boost::filesystem::exists(g_tmpFileName));
    ASSERT_EQ(boost::filesystem::file_size(boost::filesystem::path(g_tmpFileName)), g_smallFileSizeBytes);

    ASSERT_FALSE(boost::filesystem::exists(g_tmpFileName2));
    msdot::download::download_url_to_path(g_smallFileUrl, g_tmpFileName2);
    ASSERT_TRUE(boost::filesystem::exists(g_tmpFileName2));
    ASSERT_EQ(boost::filesystem::file_size(boost::filesystem::path(g_tmpFileName2)), g_smallFileSizeBytes);

    ASSERT_FALSE(boost::filesystem::exists(g_tmpFileName3));
    msdot::download::download_url_to_path(g_smallFileUrl, g_tmpFileName3);
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
            msdot::download::download_url_to_path(g_smallFileUrl, g_tmpFileName);
        }
        catch (const msdod::exception&)
        {
            ASSERT_TRUE(false);
        }
    });
    std::thread downloadThread2([&]()
    {
        try
        {
            msdot::download::download_url_to_path(g_smallFileUrl, g_tmpFileName2);
        }
        catch (const msdod::exception&)
        {
            ASSERT_TRUE(false);
        }
    });
    std::thread downloadThread3([&]()
    {
        try
        {
            msdot::download::download_url_to_path(g_smallFileUrl, g_tmpFileName3);
        }
        catch (const msdod::exception&)
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
            msdot::download::download_url_to_path(g_smallFileUrl, g_tmpFileName);
        }
        catch (const msdod::exception&)
        {
            ASSERT_TRUE(false);
        }
    });
    std::thread downloadThread2([&]()
    {
        try
        {
            msdot::download::download_url_to_path(g_largeFileUrl, g_tmpFileName2, cancelToken);
            ASSERT_TRUE(false); // Cancel will cause download_url_to_path to throw, so reaching here would be unexpected.
        }
        catch (const msdod::exception& e)
        {
            ASSERT_EQ(e.error_code().value(), static_cast<int>(std::errc::operation_canceled));
        }
    });
    std::thread downloadThread3([&]()
    {
        try
        {
            msdot::download::download_url_to_path(g_smallFileUrl, g_tmpFileName3);
        }
        catch (const msdod::exception&)
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

TEST_F(DownloadTests, FileDeletionAfterPause)
{
    auto largeDownload = msdot::download::make(g_largeFileUrl, g_tmpFileName2);
    auto cleanup = dotest::util::scope_exit([&largeDownload]()
        {
            std::cout << "Aborting download\n";
            largeDownload->abort();
        });

    largeDownload->start();
    std::this_thread::sleep_for(2s);
    largeDownload->pause();
    auto status = largeDownload->get_status();
    ASSERT_EQ(status.state(), msdo::download_state::paused) << "Download is paused";

    boost::filesystem::remove(g_tmpFileName2);
    ASSERT_FALSE(boost::filesystem::exists(g_tmpFileName2)) << "Output file deleted";

#if defined(DO_CLIENT_AGENT)
    try
    {
        largeDownload->resume();
        ASSERT_TRUE(false) << "Expected resume() to throw";
    }
    catch (const msdod::exception& ex)
    {
        ASSERT_EQ(ex.error_code().value(), DO_ERROR_FROM_SYSTEM_ERROR(ENOENT)) << "Resume failed due to missing output file";
    }
#else
    TestHelpers::DeleteDoSvcTemporaryFiles(g_tmpFileName2);
    largeDownload->resume();
    TestHelpers::WaitForState(*largeDownload, msdo::download_state::paused);
    status = largeDownload->get_status();
    ASSERT_EQ(status.error_code().value(), static_cast<int>(0x80070002));
#endif
}

#if defined(DO_INTERFACE_REST)

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
        msdot::download::download_url_to_path(g_smallFileUrl, g_tmpFileName);
        ASSERT_TRUE(!"Expected operation to throw exception");
    }
    catch (const msdod::exception& e)
    {
        ASSERT_EQ(e.error_code().value(), static_cast<int>(msdo::errc::no_service));
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
        msdot::download::download_url_to_path(g_smallFileUrl, g_tmpFileName);
        ASSERT_TRUE(!"Expected operation to throw exception");
    }
    catch (const msdod::exception& e)
    {
        ASSERT_EQ(e.error_code().value(), static_cast<int>(msdo::errc::no_service));
    }
    ASSERT_FALSE(boost::filesystem::exists(g_tmpFileName));
}

TEST_F(DownloadTests, MultipleRestPortFileExists_Download)
{
    TestHelpers::StopService("deliveryoptimization-agent.service");
    auto startService = dotest::util::scope_exit([]()
        {
            TestHelpers::StartService("deliveryoptimization-agent.service");
        });
    TestHelpers::CreateRestPortFiles(5);
    ASSERT_GE(TestHelpers::CountRestPortFiles(), 5u);
    startService.reset();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    ASSERT_EQ(TestHelpers::CountRestPortFiles(), 1u) << "All other restport files must be deleted by the agent";

    ASSERT_FALSE(boost::filesystem::exists(g_tmpFileName));
    msdot::download::download_url_to_path(g_smallFileUrl, g_tmpFileName);
    ASSERT_TRUE(boost::filesystem::exists(g_tmpFileName));
    ASSERT_EQ(boost::filesystem::file_size(boost::filesystem::path(g_tmpFileName)), g_smallFileSizeBytes);
}

#endif // DO_INTERFACE_REST
