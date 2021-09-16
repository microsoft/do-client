#ifndef _DELIVERY_OPTIMIZATION_TESTS_COMMON_H
#define _DELIVERY_OPTIMIZATION_TESTS_COMMON_H

// 'U' macro has a conflicting definition between gtest headers and cpprestsdk headers, so undef here:
#undef U
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#endif
