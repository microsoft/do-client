// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "tests_common.h"

#include <atomic>
#include <thread>

#include <boost/filesystem.hpp>

#include "do_download.h"
#include "do_download_status.h"
#include "do_errors.h"
#include "test_data.h"
#include "test_helpers.h"

namespace msdo = microsoft::deliveryoptimization;
using namespace std::chrono_literals; // NOLINT(build/namespaces)

// These tests cause build pipeline failure due to cutting off communication between the agent and backend pipeline infra.
// TODO(jimson): Look into moving network connectivity tests to E2E tests when that infrastructure has been set up
class NetworkConnectivityTests : public ::testing::Test
{
public:
    void SetUp() override;
    void TearDown() override;
};

void NetworkConnectivityTests::SetUp()
{
    TestHelpers::CleanTestDir();
    TestHelpers::EnableNetwork();
}

void NetworkConnectivityTests::TearDown()
{
    TestHelpers::CleanTestDir();
    TestHelpers::EnableNetwork();
}

TEST_F(NetworkConnectivityTests, DISABLED_SimpleBlockingDownloadNetworkDisconnect)
{
    std::thread downloadThread([&]()
        {
            try
            {
                msdo::download::download_url_to_path(g_largeFileUrl, g_tmpFileName, 60s);
                ASSERT_TRUE(false);
            }
            catch (const msdo::exception& e)
            {
                ASSERT_EQ(e.error_code().value(), DO_ERROR_FROM_STD_ERROR(std::errc::timed_out));
            }
        });
    TestHelpers::DisableNetwork();
    std::this_thread::sleep_for(90s);
    downloadThread.join();
}

TEST_F(NetworkConnectivityTests, DISABLED_SimpleBlockingDownloadNetworkReconnect)
{
    std::thread downloadThread([&]()
        {
            try
            {
                msdo::download::download_url_to_path(g_largeFileUrl, g_tmpFileName);
                ASSERT_EQ(boost::filesystem::file_size(boost::filesystem::path(g_tmpFileName)), g_largeFileSizeBytes);
            }
            catch (const msdo::exception& e)
            {
                ASSERT_TRUE(false);
            }
        });
    TestHelpers::DisableNetwork();
    std::this_thread::sleep_for(60s);
    TestHelpers::EnableNetwork();
    downloadThread.join();
}