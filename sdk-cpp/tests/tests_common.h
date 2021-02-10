#pragma once

// 'U' macro has a conflicting definition between gtest headers and cpprestsdk headers, so undef here:
#undef U
#include <gtest/gtest.h>
#include <gmock/gmock.h>