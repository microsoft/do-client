// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "test_helpers.h"

#include <boost/filesystem.hpp>

#include "test_data.h"

void TestHelpers::CleanupWorkingDir()
{
    return;
}

void TestHelpers::CleanTestDir()
{
    if (boost::filesystem::exists(g_tmpFileName))
    {
        boost::filesystem::remove(g_tmpFileName);
    }
    if (boost::filesystem::exists(g_tmpFileName2))
    {
        boost::filesystem::remove(g_tmpFileName2);
    }
    if (boost::filesystem::exists(g_tmpFileName3))
    {
        boost::filesystem::remove(g_tmpFileName3);
    }
}