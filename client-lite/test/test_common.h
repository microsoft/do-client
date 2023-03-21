// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "do_common.h"

#include <chrono>
#include <filesystem>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

extern const std::filesystem::path g_testTempDir;

inline void ClearTestTempDir()
{
    if (std::filesystem::exists(g_testTempDir))
    {
        std::filesystem::remove_all(g_testTempDir);
    }
    std::filesystem::create_directories(g_testTempDir);
}
