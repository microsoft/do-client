// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "tests_common.h"

#include <chrono>
#include <functional>
#include <thread>

#include <boost/filesystem.hpp>

#include "do_download.h"
#include "do_errors.h"
#include "test_data.h"
#include "test_helpers.h"

namespace msdo = microsoft::deliveryoptimization;
using namespace std::chrono_literals;

class DownloadTestsDOSVC : public ::testing::Test
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

// In the new COM interface we allow for downloading to the same destination - it overwrites and cancels the first download
TEST_F(DownloadTestsDOSVC, Download1PausedDownload2SameDestTest)
{
    ASSERT_FALSE(boost::filesystem::exists(g_tmpFileName));

    std::unique_ptr<msdo::download> simpleDownload;
    auto ec = msdo::download::make_nothrow(g_largeFileUrl, g_tmpFileName, simpleDownload);
    ASSERT_TRUE(!ec);

    // msdo::download_property_value propUri;
    // ASSERT_TRUE(!msdo::download_property_value::make_nothrow(g_largeFileUrl, propUri));
    // ASSERT_TRUE(!simpleDownload->set_property_nothrow(msdo::download_property::uri, propUri));

    // msdo::download_property_value propFilePath;
    // ASSERT_TRUE(!msdo::download_property_value::make_nothrow(g_tmpFileName, propFilePath));
    // ASSERT_TRUE(!simpleDownload->set_property_nothrow(msdo::download_property::uri, propFilePath));

    msdo::download_status status;
    simpleDownload->get_status_nothrow(status);
    ASSERT_EQ(status.state(), msdo::download_state::created);
    ASSERT_EQ(status.bytes_transferred(), 0u);

    simpleDownload->start_nothrow();
    std::this_thread::sleep_for(1s);
    simpleDownload->pause_nothrow();
    TestHelpers::WaitForState(*simpleDownload, msdo::download_state::paused);

    msdo::download::download_url_to_path_nothrow(g_smallFileUrl, g_tmpFileName);
    ASSERT_EQ(boost::filesystem::file_size(boost::filesystem::path(g_tmpFileName)), g_smallFileSizeBytes);
}