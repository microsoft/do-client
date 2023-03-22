// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "do_common.h"
#include "do_filesystem.h"

#include <chrono>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

extern const fs::path g_testTempDir;

inline void ClearTestTempDir()
{
    if (fs::exists(g_testTempDir))
    {
        fs::remove_all(g_testTempDir);
    }
    fs::create_directories(g_testTempDir);
}
