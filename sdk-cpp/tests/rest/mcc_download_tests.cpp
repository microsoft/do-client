// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "tests_common.h"

#include <filesystem>
#include <random>
#include <thread>
#include "do_config.h"
#include "do_download.h"
#include "do_download_status.h"
#include "do_errors.h"
#include "do_persistence.h"
#include "test_data.h"
#include "test_helpers.h"

namespace msdo = microsoft::deliveryoptimization;
namespace msdod = microsoft::deliveryoptimization::details;
namespace msdot = microsoft::deliveryoptimization::test;

class MCCDownloadTests : public ::testing::Test
{
public:
    void SetUp() override
    {
        TestHelpers::CleanTestDir();
        if (std::filesystem::exists(msdod::GetConfigFilePath()))
        {
            std::filesystem::remove(msdod::GetConfigFilePath());
        }
    }

    void TearDown() override
    {
        SetUp();
        // Make sure docs reloads config immediately
        TestHelpers::RestartService(g_docsSvcName);
    }
};

TEST_F(MCCDownloadTests, DownloadWithMockIoTConnectionString)
{
    // Mock connecting string with GatewayHostName pointing to a valid MCC instance
    std::string iotConnString("HostName=instance-company-iothub-ver.host.tld;DeviceId=user-dev-name;SharedAccessKey=abcdefghijklmnopqrstuvwxyzABCDE123456789012=;GatewayHostName=");
    iotConnString += g_mccHostName;
    std::cout << "Using mock IoT connections string: " << iotConnString << '\n';
    int ret = deliveryoptimization_set_iot_connection_string(iotConnString.data());
    ASSERT_EQ(ret, 0);

    msdot::download::download_url_to_path(g_prodFileUrl, g_tmpFileName);
    // TODO(jimson): Parse docs logs and check download occurred via MCC
}

TEST_F(MCCDownloadTests, DownloadWithMockIoTConnectionStringInvalidHost)
{
    // Mock connecting string with GatewayHostName pointing to an invalid MCC instance
    const char* expectedValue = "HostName=instance-company-iothub-ver.host.tld;DeviceId=user-dev-name;SharedAccessKey=abcdefghijklmnopqrstuvwxyzABCDE123456789012=;GatewayHostName=ahdkhkasdhaksd";
    int ret = deliveryoptimization_set_iot_connection_string(expectedValue);
    ASSERT_EQ(ret, 0);

    // Download should still succeed in a reasonable time
    msdo::download::download_url_to_path(g_prodFileUrl, g_tmpFileName, std::chrono::seconds(60));
}

TEST_F(MCCDownloadTests, DownloadWithInvalidHostAndUrl)
{
    const char* expectedValue = "GatewayHostName=ahdkhkasdhaksd";
    int ret = deliveryoptimization_set_iot_connection_string(expectedValue);
    ASSERT_EQ(ret, 0);

    std::random_device rd;
    std::mt19937_64 gen{rd()};
    const auto invalidUrl = "http://" + std::to_string(gen()) + ".com";
    std::cout << "Using invalid URL: " << invalidUrl << std::endl;

    auto download = msdot::download::make(invalidUrl, g_tmpFileName);
    download->start();

    // Wait enough time to exercise the agent code that attempts both MCC and CDN in a loop.
    // Verify there is no progress while waiting.
    const auto timeout = std::chrono::seconds(90);
    const auto endTime = std::chrono::steady_clock::now() + timeout;
    uint64_t bytesTransferred = download->get_status().bytes_transferred();
    std::cout << "Verifying there is no download progress for " << timeout.count() << " seconds" << std::endl;
    do
    {
        std::this_thread::sleep_for(std::chrono::seconds(2));

        const auto newStatus = download->get_status();
        ASSERT_EQ(newStatus.state(), msdo::download_state::transferring);
        ASSERT_EQ(newStatus.bytes_transferred(), bytesTransferred);

    } while (std::chrono::steady_clock::now() < endTime);

    download->abort();
}
