#include "test_common.h"
#include "mcc_manager.h"

#include <chrono>
#include <fstream>
#include <iostream>

#include <boost/asio.hpp>
#include <cpprest/json.h>

#include "config_defaults.h"
#include "config_manager.h"
#include "do_error.h"
#include "do_test_helpers.h"
#include "download_manager.h"
#include "download_status.h"
#include "stop_watch.h"
#include "test_data.h"
#include "test_verifiers.h"

using namespace std::chrono_literals; // NOLINT(build/namespaces)
using btcp_t = boost::asio::ip::tcp;

#define TEST_MOCK_MCC_HOST   "10.130.48.179"
#define TEST_MOCK_MCC_HOST2  "10.130.48.180"

const cppfs::path g_adminConfigFilePath = g_testTempDir / "admin-config.json";
const cppfs::path g_sdkConfigFilePath = g_testTempDir / "sdk-config.json";

static void SetIoTConnectionString(const char* connectionString)
{
    web::json::value json;
    json[ConfigName_AduIoTConnectionString] = web::json::value(connectionString);
    utility::string_t jsonStr = json.serialize();

    std::ofstream writer;
    writer.exceptions(std::fstream::badbit | std::fstream::failbit);
    writer.open(g_sdkConfigFilePath.string(), std::ios_base::out | std::ios_base::trunc);
    writer << jsonStr.c_str();
}

static void SetDOCacheHostConfig(const char* server)
{
    web::json::value json;
    json[ConfigName_CacheHostServer] = web::json::value(server);
    utility::string_t jsonStr = json.serialize();

    std::ofstream writer;
    writer.exceptions(std::fstream::badbit | std::fstream::failbit);
    writer.open(g_adminConfigFilePath.string(), std::ios_base::out | std::ios_base::app);
    writer << jsonStr;
}

static void SetFallbackDelayConfig(std::chrono::seconds delay)
{
    web::json::value json;
    json[ConfigName_CacheHostFallbackDelayFgSecs] = web::json::value(delay.count());
    utility::string_t jsonStr = json.serialize();

    std::ofstream writer;
    writer.exceptions(std::fstream::badbit | std::fstream::failbit);
    writer.open(g_adminConfigFilePath.string(), std::ios_base::out | std::ios_base::app);
    writer << jsonStr;
}

class MCCManagerTests : public ::testing::Test
{
public:
    void SetUp() override
    {
        if (cppfs::exists(g_adminConfigFilePath))
        {
            cppfs::remove(g_adminConfigFilePath);
        }
        if (cppfs::exists(g_sdkConfigFilePath))
        {
            cppfs::remove(g_sdkConfigFilePath);
        }
    }

    void TearDown() override
    {
        SetUp();
    }

protected:
    void _VerifyExpectedCacheHost(const std::string& expectedHostValue)
    {
        ConfigManager configReader(g_adminConfigFilePath.string(), g_sdkConfigFilePath.string());
        MCCManager mccManager(configReader);
        std::string mccHost = mccManager.NextHost();
        ASSERT_EQ(mccHost, expectedHostValue);
    }
};

TEST_F(MCCManagerTests, ParseIoTConnectionString)
{
    // Gateway specified as the last element
    // [SuppressMessage("Microsoft.Security", "CS002:SecretInNextLine", Justification="Not credentials (false positive)")]
    SetIoTConnectionString("HostName=instance-company-iothub-ver.host.tld;DeviceId=user-dev-name;SharedAccessKey=abcdefghijklmnopqrstuvwxyzABCDE123456789012=;GatewayHostName=" TEST_MOCK_MCC_HOST);
    _VerifyExpectedCacheHost(TEST_MOCK_MCC_HOST);

    // Gateway specified in the middle
    SetIoTConnectionString("HostName=instance-company-iothub-ver.host.tld;GatewayHostName=" TEST_MOCK_MCC_HOST ";DeviceId=user-dev-name;SharedAccessKey=abcdefghijklmnopqrstuvwxyzABCDE123456789012=");
    _VerifyExpectedCacheHost(TEST_MOCK_MCC_HOST);

    // No gateway specified
    cppfs::remove(g_sdkConfigFilePath);
    _VerifyExpectedCacheHost("");
}

TEST_F(MCCManagerTests, AdminConfigOverride)
{
    // [SuppressMessage("Microsoft.Security", "CS002:SecretInNextLine", Justification="Not credentials (false positive)")]
    SetIoTConnectionString("HostName=instance-company-iothub-ver.host.tld;DeviceId=user-dev-name;SharedAccessKey=abcdefghijklmnopqrstuvwxyzABCDE123456789012=;GatewayHostName=" TEST_MOCK_MCC_HOST);
    SetDOCacheHostConfig(TEST_MOCK_MCC_HOST2);
    _VerifyExpectedCacheHost(TEST_MOCK_MCC_HOST2);

    cppfs::remove(g_adminConfigFilePath);
    _VerifyExpectedCacheHost(TEST_MOCK_MCC_HOST);
}

// Disabled tests: Azure lab MCC instance isn't responding quickly with 404.
// Running it locally: ./deliveryoptimization-agent-tests --gtest_also_run_disabled_tests --gtest_filter=*DISABLED_Download404WithFallback -m <MCC-host>
TEST_F(MCCManagerTests, DISABLED_Download404WithFallback)
{
    SetDOCacheHostConfig(g_mccHostName.data());
    const auto fallbackDelay = std::chrono::seconds{10};
    SetFallbackDelayConfig(fallbackDelay);

    ConfigManager configs(g_adminConfigFilePath.string(), g_sdkConfigFilePath.string());
    DownloadManager manager(configs);
    const std::string destFile = g_testTempDir / "prodfile.test";
    const std::string id = manager.CreateDownload(g_404Url, destFile); // start with 404 url
    manager.StartDownload(id);
    auto sw = StopWatch::StartNew();
    auto endTime = std::chrono::steady_clock::now() + fallbackDelay + std::chrono::seconds{5};
    while ((std::chrono::steady_clock::now() < endTime) &&
        (manager.GetDownloadStatus(id).State == DownloadState::Transferring))
    {
        std::this_thread::sleep_for(1s);
    }
    sw.Stop();

    auto status = manager.GetDownloadStatus(id);
    VerifyError(status, HTTP_E_STATUS_NOT_FOUND);
    ASSERT_GE(sw.GetElapsedInterval(), fallbackDelay) << "Fatal error raised only after fallback delay";

    manager.SetDownloadProperty(id, DownloadProperty::Uri, g_prodFileUrl);
    manager.StartDownload(id);
    endTime = std::chrono::steady_clock::now() + fallbackDelay + std::chrono::seconds{60};
    status = manager.GetDownloadStatus(id);
    while ((std::chrono::steady_clock::now() < endTime) && (status.State == DownloadState::Transferring))
    {
        std::this_thread::sleep_for(1s);
        status = manager.GetDownloadStatus(id);
    }
    ASSERT_EQ(status.State, DownloadState::Transferred) << "Download completed eventually";
    ASSERT_EQ(status.BytesTransferred, g_prodFileSizeBytes);
    VerifyDownloadComplete(manager, id, g_prodFileSizeBytes);
    manager.FinalizeDownload(id);
}

TEST_F(MCCManagerTests, DISABLED_Download404NoFallback)
{
    SetDOCacheHostConfig(g_mccHostName.data());
    SetFallbackDelayConfig(g_cacheHostFallbackDelayNoFallback);

    ConfigManager configs(g_adminConfigFilePath.string(), g_sdkConfigFilePath.string());
    DownloadManager manager(configs);
    const std::string destFile = g_testTempDir / "prodfile.test";
    const std::string id = manager.CreateDownload(g_404Url, destFile); // start with 404 url
    manager.StartDownload(id);

    auto sw = StopWatch::StartNew();
    auto endTime = std::chrono::steady_clock::now() + std::chrono::minutes{2};
    while ((std::chrono::steady_clock::now() < endTime) &&
        (manager.GetDownloadStatus(id).State == DownloadState::Transferring))
    {
        std::this_thread::sleep_for(1s);
    }
    sw.Stop();

    auto status = manager.GetDownloadStatus(id);
    VerifyError(status, DO_E_DOWNLOAD_NO_PROGRESS, HTTP_E_STATUS_NOT_FOUND);
    ASSERT_GE(sw.GetElapsedInterval(), g_progressTrackerMaxNoProgressIntervalsWithMCC * g_progressTrackerCheckInterval)
        << "Fatal error raised only after the MCC no-progress delay";
    manager.AbortDownload(id);
}

// Set invalid MCC host and no-fallback config. Expect download to timeout.
TEST_F(MCCManagerTests, NoFallbackDownload)
{
    // [SuppressMessage("Microsoft.Security", "CS002:SecretInNextLine", Justification="Not credentials (false positive)")]
    SetIoTConnectionString("HostName=instance-company-iothub-ver.host.tld;DeviceId=user-dev-name;SharedAccessKey=abcdefghijklmnopqrstuvwxyzABCDE123456789012=;GatewayHostName=ahdkhkasdhaksd");
    SetFallbackDelayConfig(g_cacheHostFallbackDelayNoFallback);

    ConfigManager configs(g_adminConfigFilePath.string(), g_sdkConfigFilePath.string());
    DownloadManager manager(configs);
    const std::string destFile = g_testTempDir / "prodfile.test";
    const std::string id = manager.CreateDownload(g_prodFileUrl, destFile);
    manager.StartDownload(id);
    std::this_thread::sleep_for(2s);

    const auto endTime = std::chrono::steady_clock::now() + std::chrono::seconds(60);
    auto status = manager.GetDownloadStatus(id);
    while ((status.State == DownloadState::Transferring) && (std::chrono::steady_clock::now() < endTime))
    {
        std::this_thread::sleep_for(1s);
        status = manager.GetDownloadStatus(id);
    }

    ASSERT_EQ(status.State, DownloadState::Transferring) << "Download still in transferring state";
    ASSERT_EQ(status.BytesTransferred, 0) << "No bytes downloaded due to invalid cache host and no fallback";
    manager.AbortDownload(id);
}

// Set invalid MCC host and fallback config. Expect download to complete only after the fallback delay.
// Note: This test fails on WSL (Ubuntu 18.04) because the bad host name seems to be sticking at the DNS query layer leading
// to the new valid host name not being picked up even after we switch to it. Cpprestsdk uses Boost's resolver
// and the issue on WSL can be reproduced with the BoostResolver* tests below. Test works fine on Ubuntu 18.04 VM.
TEST_F(MCCManagerTests, FallbackWithDelayDownload)
{
    // [SuppressMessage("Microsoft.Security", "CS002:SecretInNextLine", Justification="Not credentials (false positive)")]
    SetIoTConnectionString("HostName=instance-company-iothub-ver.host.tld;DeviceId=user-dev-name;SharedAccessKey=abcdefghijklmnopqrstuvwxyzABCDE123456789012=;GatewayHostName=ahdkhkasdhaksd");

    const auto fallbackDelay = 45s;
    SetFallbackDelayConfig(fallbackDelay);

    ConfigManager configs(g_adminConfigFilePath.string(), g_sdkConfigFilePath.string());
    DownloadManager manager(configs);
    const std::string destFile = g_testTempDir / "prodfile.test";
    const std::string id = manager.CreateDownload(g_prodFileUrl, destFile);
    manager.StartDownload(id);
    const auto startTime = std::chrono::steady_clock::now();
    std::this_thread::sleep_for(2s);

    // g_progressTrackerMaxRetryDelay used because client retry delay doesn't take into
    // account the time left to fallback. Hence it could wait an extra retry interval before falling back.
    const auto endTime = std::chrono::steady_clock::now() + fallbackDelay + g_progressTrackerMaxRetryDelay + 10s;
    auto status = manager.GetDownloadStatus(id);
    while ((status.State != DownloadState::Transferred) && (status.State != DownloadState::Paused)
        && (std::chrono::steady_clock::now() < endTime))
    {
        std::this_thread::sleep_for(1s);
        status = manager.GetDownloadStatus(id);
    }

    ASSERT_EQ(status.State, DownloadState::Transferred) << "Download completed eventually";
    ASSERT_EQ(status.BytesTransferred, g_prodFileSizeBytes);
    VerifyDownloadComplete(manager, id, g_prodFileSizeBytes);

    const auto timeTaken = std::chrono::steady_clock::now() - startTime;
    ASSERT_GE(timeTaken, fallbackDelay + 2s) << "Download completed only after the fallback delay";

    manager.FinalizeDownload(id);
}

TEST_F(MCCManagerTests, BoostResolverGoodQuery)
{
    dotest::util::BoostAsioWorker asioService;

    btcp_t::resolver::query goodQuery("dl.delivery.mp.microsoft.com", "80");
    const auto spEndpoint = asioService.ResolveDnsQuery(goodQuery);
    ASSERT_TRUE(spEndpoint) << "Found at least one address";
}

TEST_F(MCCManagerTests, BoostResolverQuery)
{
    dotest::util::BoostAsioWorker asioService;

    std::cout << "Issuing the bad query\n";
    btcp_t::resolver::query badQuery("ahdkhkasdhaksd", "80");
    auto spEndpoint = asioService.ResolveDnsQuery(badQuery);
    ASSERT_FALSE(spEndpoint) << "Found no addresses for the bad query";

    std::cout << "\nIssuing the good query in 5 seconds\n";
    std::this_thread::sleep_for(std::chrono::seconds(5));
    btcp_t::resolver::query goodQuery("dl.delivery.mp.microsoft.com", "80");
    spEndpoint = asioService.ResolveDnsQuery(goodQuery);
    ASSERT_TRUE(spEndpoint) << "Found at least one address for the good query";
}
