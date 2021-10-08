// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "tests_common.h"

#include "do_download_status.h"
#include "do_errors.h"

namespace msdo = microsoft::deliveryoptimization;

TEST(DownloadStatusTests, TestAccessors)
{
    msdo::download_status status;
    ASSERT_EQ(status.bytes_total(), 0);
    ASSERT_EQ(status.bytes_transferred(), 0);
    ASSERT_EQ(status.error_code(), 0);
    ASSERT_EQ(status.extended_error_code(), 0);
    ASSERT_EQ(status.state(), msdo::download_state::created);

    status = msdo::download_status(100, 50, -1, -1, msdo::download_state::transferring);
    ASSERT_EQ(status.bytes_total(), 100);
    ASSERT_EQ(status.bytes_transferred(), 50);
    ASSERT_EQ(status.error_code(), -1);
    ASSERT_EQ(status.extended_error_code(), -1);
    ASSERT_EQ(status.state(), msdo::download_state::transferring);
}

TEST(DownloadStatusTests, TestIsError)
{
    msdo::download_status status(0, 0, -1, 0, msdo::download_state::created);
    ASSERT_TRUE(status.is_error());

    status = msdo::download_status(0, 0, 0, 0, msdo::download_state::created);
    ASSERT_FALSE(status.is_error());
}

TEST(DownloadStatusTests, TestIsTransientError)
{
    msdo::download_status status(0, 0, 0, -1, msdo::download_state::paused);
    ASSERT_TRUE(status.is_transient_error());

    status = msdo::download_status(0, 0, -1, -1, msdo::download_state::created);
    ASSERT_FALSE(status.is_transient_error());

    status = msdo::download_status(0, 0, -1, 0, msdo::download_state::created);
    ASSERT_FALSE(status.is_transient_error());
}

TEST(DownloadStatusTests, TestIsComplete)
{
    msdo::download_status status(100, 100, 0, 0, msdo::download_state::transferred);
    ASSERT_TRUE(status.is_complete());

    status = msdo::download_status(100, 100, -1, -1, msdo::download_state::transferred);
    ASSERT_TRUE(status.is_complete());

    status = msdo::download_status(100, 99, 0, -1, msdo::download_state::transferring);
    ASSERT_FALSE(status.is_complete());

    status = msdo::download_status(100, 0, 0, 0, msdo::download_state::created);
    ASSERT_FALSE(status.is_complete());
}
