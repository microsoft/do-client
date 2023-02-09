// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "tests_common.h"

#include <cinttypes>
#include <chrono>
#include <thread>

#include "do_download.h"
#include "do_errors.h"
#include "test_data.h"
#include "test_helpers.h"

namespace msdo = microsoft::deliveryoptimization;
namespace msdot = microsoft::deliveryoptimization::test;
using namespace std::chrono_literals;

class DownloadPropertyTests : public ::testing::Test
{
public:
    void SetUp() override
    {
        TestHelpers::CleanTestDir();
        ASSERT_FALSE(boost::filesystem::exists(g_tmpFileName));
    }
    void TearDown() override
    {
        TestHelpers::CleanTestDir();
    }

#ifdef DO_CLIENT_DOSVC
    // Our build/test machines run Windows Server 2019, which use an older COM interface and does not support setting IntegrityCheckInfo through DODownloadProperty com interface
    // Accept multiple error codes to handle running tests both locally and on the build machine
    static void VerifyError(int32_t code, const std::vector<int32_t>& expectedErrors)
    {
        ASSERT_TRUE(std::find(expectedErrors.begin(), expectedErrors.end(), code) != expectedErrors.end());
    }
#endif
};

// Only the DoSvc client (windows) supports get/set property at present
#ifdef DO_CLIENT_DOSVC

static uint32_t TimeOperation(const std::function<void()>& op)
{
    auto start = std::chrono::steady_clock::now();
    op();
    auto end = std::chrono::steady_clock::now();
    return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());
}

TEST_F(DownloadPropertyTests, SmallDownloadSetCallerNameTest)
{
    auto simpleDownload = msdot::download::make(g_smallFileUrl, g_tmpFileName);

    std::string strCallerName("dosdkcpp_tests");
    simpleDownload->set_property(msdo::download_property::caller_name, strCallerName);

    std::string outCallerName;
    simpleDownload->get_property(msdo::download_property::caller_name, outCallerName);
    ASSERT_EQ(strCallerName, outCallerName);

    simpleDownload->start_and_wait_until_completion();
    ASSERT_TRUE(boost::filesystem::exists(g_tmpFileName));
}

TEST_F(DownloadPropertyTests, SmallDownloadWithPhfDigestandCvTest)
{
    std::unique_ptr<msdo::download> simpleDownload;
    ASSERT_EQ(msdo::download::make(g_smallFileUrl, g_tmpFileName, simpleDownload).value(), 0);

    std::vector<int32_t> expectedErrors = { 0, static_cast<int32_t>(msdo::errc::do_e_unknown_property_id) };

    int32_t code = simpleDownload->set_property(msdo::download_property::integrity_check_mandatory, false).value();
    VerifyError(code, expectedErrors);

    code = simpleDownload->set_property(msdo::download_property::integrity_check_info, g_smallFilePhfInfoJson).value();
    VerifyError(code, expectedErrors);

    code = simpleDownload->set_property(msdo::download_property::correlation_vector, "g+Vo71JZwkmJdYfF.0").value();
    VerifyError(code, expectedErrors);

    ASSERT_EQ(simpleDownload->start_and_wait_until_completion().value(), 0);

    ASSERT_TRUE(boost::filesystem::exists(g_tmpFileName));
}

TEST_F(DownloadPropertyTests, InvalidPhfDigestTest)
{
    std::unique_ptr<msdo::download> simpleDownload;
    ASSERT_EQ(msdo::download::make(g_smallFileUrl, g_tmpFileName, simpleDownload).value(), 0);

    std::vector<int32_t> expectedErrors = { 0, static_cast<int32_t>(msdo::errc::invalid_arg) };

    int32_t code = simpleDownload->set_property(msdo::download_property::integrity_check_info, "blah").value();
    VerifyError(code, expectedErrors);
}

TEST_F(DownloadPropertyTests, SmallDownloadWithCustomHeaders)
{
    auto simpleDownload = msdot::download::make(g_smallFileUrl, g_tmpFileName);
    simpleDownload->set_property(msdo::download_property::http_custom_headers, "XCustom1=someData\nXCustom2=moreData\n");
    simpleDownload->start_and_wait_until_completion();
    ASSERT_TRUE(boost::filesystem::exists(g_tmpFileName));
}

TEST_F(DownloadPropertyTests, CallbackTestUseDownload)
{
    auto simpleDownload = msdot::download::make(g_largeFileUrl, g_tmpFileName);

    bool fPauseDownload = false;
    auto callback = [&fPauseDownload](msdo::download& download, msdo::download_status& status)
        {
            char msgBuf[1024];
            snprintf(msgBuf, sizeof(msgBuf), "Received status callback: %" PRIu64 "/%" PRIu64 ", 0x%x, 0x%x, %u",
                status.bytes_transferred(), status.bytes_total(), status.error_code().value(), status.extended_error_code().value(),
                static_cast<unsigned int>(status.state()));
            std::cout << msgBuf << std::endl;

            msdo::download_status status2;
            ASSERT_EQ(download.get_status(status2).value(), 0);
            if (fPauseDownload)
            {
                ASSERT_EQ(download.pause().value(), 0);
            }
        };

    simpleDownload->set_status_callback(callback);
    simpleDownload->start();
    std::this_thread::sleep_for(5s);
    fPauseDownload = true;

    TestHelpers::WaitForState(*simpleDownload, msdo::download_state::paused);
}

TEST_F(DownloadPropertyTests, SetCallbackTest)
{
    auto simpleDownload = msdot::download::make(g_smallFileUrl, g_tmpFileName);

    int i= 0;
    auto callback = [&i](msdo::download&, msdo::download_status& status)
        {
            char msgBuf[1024];
            snprintf(msgBuf, sizeof(msgBuf), "Received status callback: %" PRIu64 "/%" PRIu64 ", 0x%x, 0x%x, %u",
                status.bytes_transferred(), status.bytes_total(), status.error_code().value(), status.extended_error_code().value(),
                static_cast<unsigned int>(status.state()));
            std::cout << msgBuf << std::endl;
            i += 1;
        };
    simpleDownload->set_status_callback(callback);

    simpleDownload->start_and_wait_until_completion();

    ASSERT_GE(i, 0);
}

TEST_F(DownloadPropertyTests, OverrideCallbackTest)
{
    auto simpleDownload = msdot::download::make(g_smallFileUrl, g_tmpFileName);
    int i = 0;
    simpleDownload->set_status_callback([&i](msdo::download&, msdo::download_status&){ i += 1; });
    simpleDownload->set_status_callback([](msdo::download&, msdo::download_status&){});
    simpleDownload->start_and_wait_until_completion();
    ASSERT_EQ(i, 0);
}

TEST_F(DownloadPropertyTests, ForegroundBackgroundRace)
{
    uint32_t backgroundDuration = TimeOperation([&]()
        {
            auto simpleDownload = msdot::download::make(g_largeFileUrl, g_tmpFileName);

            simpleDownload->set_property(msdo::download_property::use_foreground_priority, false);

            bool outVal = true;
            simpleDownload->get_property(msdo::download_property::use_foreground_priority, outVal);
            ASSERT_FALSE(outVal);

            simpleDownload->start_and_wait_until_completion();
        });

    printf("Time for background download: %u ms\n", backgroundDuration);
    uint32_t foregroundDuration = TimeOperation([&]()
        {
            auto simpleDownload = msdot::download::make(g_largeFileUrl, g_tmpFileName2);

            simpleDownload->set_property(msdo::download_property::use_foreground_priority, true);

            bool outVal = false;
            simpleDownload->get_property(msdo::download_property::use_foreground_priority, outVal);
            ASSERT_TRUE(outVal);

            simpleDownload->start_and_wait_until_completion();
        });
    printf("Time for foreground download: %u ms\n", foregroundDuration);

    ASSERT_LT(foregroundDuration, backgroundDuration);
}

TEST_F(DownloadPropertyTests, BasicStreamTest)
{
    auto simpleDownload = msdot::download::make(g_smallFileUrl);

    uint64_t downloadedBytes = 0;
    simpleDownload->set_output_stream([&downloadedBytes](const unsigned char*, size_t cb) -> std::error_code { downloadedBytes += cb; return DO_OK; });

    simpleDownload->start();
    TestHelpers::WaitForState(*simpleDownload, msdo::download_state::transferred);

    ASSERT_GT(downloadedBytes, 0);

    uint64_t fileSize = 0;
    simpleDownload->get_property(msdo::download_property::total_size_bytes, fileSize);
    ASSERT_EQ(downloadedBytes, fileSize);

    simpleDownload->finalize();
}

#elif defined(DO_CLIENT_AGENT)

TEST_F(DownloadPropertyTests, SmallDownloadSetCallerNameFailureTest)
{
    msdo::download_property_value callerName;
    auto ec = msdo::download_property_value::make("dosdkcpp_tests", callerName);
    ASSERT_EQ(ec.value(), static_cast<int>(msdo::errc::e_not_impl));
}

#else
#error "Target client unknown"
#endif // DO_CLIENT_DOSVC
