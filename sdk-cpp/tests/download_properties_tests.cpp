// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "tests_common.h"

#include <cinttypes>
#include <chrono>
#include <functional>
#include <thread>

#include "do_download.h"
#include "do_errors.h"
#include "test_data.h"
#include "test_helpers.h"

namespace msdo = microsoft::deliveryoptimization;
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

    static void VerifyCallWithExpectedErrors(const std::function<std::error_code()>& op, const std::vector<int32_t>& expectedErrors)
    {
        auto ec = op();
        VerifyError(ec.value(), expectedErrors);
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

static std::unique_ptr<msdo::download> g_MakeDownload(const std::string& url, const std::string& destPath)
{
    std::unique_ptr<msdo::download> downloadObj;
    auto ec = msdo::download::make(url, destPath, downloadObj);
    if (ec) throw std::exception();

    // msdo::download_property_value propUri;
    // auto ec = msdo::download_property_value::make(url, propUri);
    // if (ec) throw std::exception();
    // ec = obj.set_property(msdo::download_property::uri, propUri);
    // if (ec) throw std::exception();

    // msdo::download_property_value propFilePath;
    // ec = msdo::download_property_value::make(destPath, propFilePath);
    // ec = obj.set_property(msdo::download_property::uri, propFilePath);
    // if (ec) throw std::exception();

    return downloadObj;
}

template <typename T>
static msdo::download_property_value g_MakePropertyValue(T value)
{
    msdo::download_property_value propValue;
    auto ec = msdo::download_property_value::make(value, propValue);
    if (ec)
    {
        throw std::exception();
    }
    return propValue;
}

//TODO: Not sure how much value these tests are, functional tests utilize parsing log lines to verify these properties were set, could be useful here
//At the moment, these tests are essentially just verifying that these properties can be set and download succeeds
TEST_F(DownloadPropertyTests, SmallDownloadSetCallerNameTest)
{
    auto simpleDownload = g_MakeDownload(g_smallFileUrl, g_tmpFileName);

    std::string strCallerName("dosdkcpp_tests");
    msdo::download_property_value callerName = g_MakePropertyValue(strCallerName);
    ASSERT_EQ(simpleDownload->set_property(msdo::download_property::caller_name, callerName).value(), 0);
    ASSERT_EQ(simpleDownload->start_and_wait_until_completion().value(), 0);
    ASSERT_TRUE(boost::filesystem::exists(g_tmpFileName));
}

TEST_F(DownloadPropertyTests, SmallDownloadWithPhfDigestandCvTest)
{
    auto simpleDownload = g_MakeDownload(g_smallFileUrl, g_tmpFileName);

    std::vector<int32_t> expectedErrors = { 0, static_cast<int32_t>(msdo::errc::do_e_unknown_property_id) };

    msdo::download_property_value integrityCheckMandatory = g_MakePropertyValue(false);
    int32_t code = simpleDownload->set_property(msdo::download_property::integrity_check_mandatory, integrityCheckMandatory).value();
    VerifyError(code, expectedErrors);

    msdo::download_property_value integrityCheckInfo = g_MakePropertyValue(g_smallFilePhfInfoJson);
    code = simpleDownload->set_property(msdo::download_property::integrity_check_info, integrityCheckInfo).value();
    VerifyError(code, expectedErrors);

    std::string strCorrelationVector("g+Vo71JZwkmJdYfF.0");
    msdo::download_property_value correlationVector = g_MakePropertyValue(strCorrelationVector);
    code = simpleDownload->set_property(msdo::download_property::correlation_vector, correlationVector).value();
    VerifyError(code, expectedErrors);

    ASSERT_EQ(simpleDownload->start_and_wait_until_completion().value(), 0);

    ASSERT_TRUE(boost::filesystem::exists(g_tmpFileName));
}

TEST_F(DownloadPropertyTests, InvalidPhfDigestTest)
{
    auto simpleDownload = g_MakeDownload(g_smallFileUrl, g_tmpFileName);

    msdo::download_property_value integrityCheckInfo =  g_MakePropertyValue("blah");
    std::vector<int32_t> expectedErrors = { 0, static_cast<int32_t>(msdo::errc::invalid_arg) };
    VerifyCallWithExpectedErrors([&]()
        {
            return simpleDownload->set_property(msdo::download_property::integrity_check_info, integrityCheckInfo);
        }, expectedErrors);
}

// For some reason, custom headers are getting rejected and returning E_INVALIDARG now, disabling test for now
TEST_F(DownloadPropertyTests, DISABLED_SmallDownloadWithCustomHeaders)
{
    auto simpleDownload = g_MakeDownload(g_smallFileUrl, g_tmpFileName);

    std::string strHttpCustomHeaders("XCustom1=someData\nXCustom2=moreData");
    msdo::download_property_value httpCustomHeaders = g_MakePropertyValue(strHttpCustomHeaders);
    simpleDownload->set_property(msdo::download_property::http_custom_headers, httpCustomHeaders);

    simpleDownload->start_and_wait_until_completion();

    ASSERT_TRUE(boost::filesystem::exists(g_tmpFileName));
}

TEST_F(DownloadPropertyTests, CallbackTestUseDownload)
{
    auto simpleDownload = g_MakeDownload(g_largeFileUrl, g_tmpFileName);
    bool fPauseDownload = false;

    msdo::download_property_value callback = g_MakePropertyValue([&fPauseDownload](msdo::download& download, msdo::download_status& status)
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
        });

    ASSERT_EQ(simpleDownload->set_property(msdo::download_property::callback_interface, callback).value(), 0);
    ASSERT_EQ(simpleDownload->start().value(), 0);
    std::this_thread::sleep_for(5s);
    fPauseDownload = true;

    microsoft::deliveryoptimization::test::download testDownload{std::move(simpleDownload)};
    TestHelpers::WaitForState(testDownload, msdo::download_state::paused);
}

TEST_F(DownloadPropertyTests, SetCallbackTest)
{
    auto simpleDownload = g_MakeDownload(g_smallFileUrl, g_tmpFileName);

    int i= 0;
    msdo::download_property_value callback = g_MakePropertyValue([&i](msdo::download&, msdo::download_status& status)
        {
            char msgBuf[1024];
            snprintf(msgBuf, sizeof(msgBuf), "Received status callback: %" PRIu64 "/%" PRIu64 ", 0x%x, 0x%x, %u",
                status.bytes_transferred(), status.bytes_total(), status.error_code().value(), status.extended_error_code().value(),
                static_cast<unsigned int>(status.state()));
            std::cout << msgBuf << std::endl;
            i += 1;
        });
    ASSERT_EQ(simpleDownload->set_property(msdo::download_property::callback_interface, callback).value(), 0);

    ASSERT_EQ(simpleDownload->start_and_wait_until_completion().value(), 0);

    ASSERT_GE(i, 0);
}

TEST_F(DownloadPropertyTests, OverrideCallbackTest)
{
    auto simpleDownload = g_MakeDownload(g_smallFileUrl, g_tmpFileName);

    int i = 0;
    msdo::download_property_value callback = g_MakePropertyValue([&i](msdo::download&, msdo::download_status&)
        {
            i += 1;
        });
    ASSERT_EQ(simpleDownload->set_property(msdo::download_property::callback_interface, callback).value(), 0);

    msdo::download_property_value::status_callback_t cb2 = [](msdo::download&, msdo::download_status&) {};

    callback = g_MakePropertyValue(cb2);
    ASSERT_EQ(simpleDownload->set_property(msdo::download_property::callback_interface, callback).value(), 0);

    ASSERT_EQ(simpleDownload->start_and_wait_until_completion().value(), 0);

    ASSERT_EQ(i, 0);
}

TEST_F(DownloadPropertyTests, ForegroundBackgroundRace)
{
    uint32_t backgroundDuration = TimeOperation([&]()
        {
            auto simpleDownload = g_MakeDownload(g_largeFileUrl, g_tmpFileName);

            msdo::download_property_value foregroundPriority = g_MakePropertyValue(false);
            ASSERT_EQ(simpleDownload->set_property(msdo::download_property::use_foreground_priority, foregroundPriority).value(), 0);

            ASSERT_EQ(simpleDownload->start_and_wait_until_completion().value(), 0);
        });

    printf("Time for background download: %u ms\n", backgroundDuration);
    uint32_t foregroundDuration = TimeOperation([&]()
        {
            auto simpleDownload = g_MakeDownload(g_largeFileUrl, g_tmpFileName2);

            msdo::download_property_value foregroundPriority = g_MakePropertyValue(true);
            ASSERT_EQ(simpleDownload->set_property(msdo::download_property::use_foreground_priority, foregroundPriority).value(), 0);

            ASSERT_EQ(simpleDownload->start_and_wait_until_completion().value(), 0);
        });
    printf("Time for foreground download: %u ms\n", foregroundDuration);

    ASSERT_LT(foregroundDuration, backgroundDuration);
}

#elif defined(DO_CLIENT_AGENT)

TEST_F(DownloadPropertyTests, SmallDownloadSetCallerNameFailureTest)
{
    std::string strCallerName("dosdkcpp_tests");
    msdo::download_property_value callerName;
    auto ec = msdo::download_property_value::make(strCallerName, callerName);
    ASSERT_EQ(ec.value(), static_cast<int>(msdo::errc::e_not_impl));
}

#else
#error "Target client unknown"
#endif // DO_CLIENT_DOSVC
