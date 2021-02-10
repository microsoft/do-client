#pragma once

#include "do_common.h"

#include <chrono>
#include <experimental/filesystem>

// 'U' macro has a conflicting definition between gtest headers and cpprestsdk headers, so undef here:
#undef U
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace cppfs = std::experimental::filesystem;

extern const cppfs::path g_testTempDir;

inline void ClearTempDir()
{
    if (cppfs::exists(g_testTempDir))
    {
        cppfs::remove_all(g_testTempDir);
    }
    cppfs::create_directories(g_testTempDir);
}
