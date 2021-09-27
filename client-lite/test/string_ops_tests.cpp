// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "test_common.h"

#include "do_test_helpers.h"
#include "string_ops.h"
#include "test_helpers.h"

TEST(StringOpsTest, StringPartitionBasic)
{
    auto fnTest = [](const std::string& left, const std::string& right, char separator)
    {
        auto testString = left + separator + right;
        auto parts = StringPartition(testString, separator);
        ASSERT_EQ(parts.size(), 2u);
        ASSERT_EQ(parts[0], left);
        ASSERT_EQ(parts[1], right);
    };

    fnTest("partA", "partB", ':');
    fnTest("a", "b", ':');
    fnTest("ab", "cd", ':');
}

TEST(StringOpsTest, StringPartitionValidInputWithoutSeparator)
{
    ASSERT_EQ(StringPartition("hello, world", '!').size(), 0u);
}

TEST(StringOpsTest, StringPartitionEmptyInput)
{
    ASSERT_EQ(StringPartition("", '!').size(), 0u); // empty input string
}

static void StringPartitionTester(const std::string& left, const std::string& right, char separator,
    const char* expectedStr1 = nullptr, const char* expectedStr2 = nullptr)
{
    auto testString = left + separator + right;
    auto parts = StringPartition(testString, separator);
    size_t numExpectedParts = 0;
    if (expectedStr1) { ++numExpectedParts; }
    if (expectedStr2) { ++numExpectedParts; }
    auto fnLogStr = dotest::util::scope_exit([&]()
    {
        std::cout << "Test string: \"" << testString << "\"\n";
        if (parts.size() > 0) std::cout << "Partition1: \"" << parts[0] << "\"\n";
        if (parts.size() > 1) std::cout << "Partition2: \"" << parts[1] << "\"\n";
    });
    ASSERT_EQ(parts.size(), numExpectedParts);
    if (numExpectedParts > 0) { ASSERT_EQ(parts[0], std::string{expectedStr1}); }
    if (numExpectedParts > 1) { ASSERT_EQ(parts[1], std::string{expectedStr2}); }
    fnLogStr.release(); // no need to output if no failure
};

TEST(StringOpsTest, StringPartitionEmptySubStrings)
{
    StringPartitionTester("", "", '-'); // input string containing just the separator
}

TEST(StringOpsTest, StringPartitionOneEmptySubString)
{
    StringPartitionTester("a", "", '-', "a");
    StringPartitionTester("ab", "", '-', "ab");
    StringPartitionTester("", "c", '-', "c");
    StringPartitionTester("", "cd", '-', "cd");
}

TEST(StringOpsTest, StringPartitionEdgeCases)
{
    StringPartitionTester("ab", "c", '-', "ab", "c");
    StringPartitionTester("a", "cd", '-', "a", "cd");
}

TEST(StringOpsTest, StringPartitionInputWithMultipleInstancesOfSeparator)
{
    StringPartitionTester("part:A", "partB", ':', "part", "A:partB"); // left side includes the separator
    StringPartitionTester("partA", "par:tB", ':', "partA", "par:tB"); // right side includes the separator
    StringPartitionTester("part:A", "part:B", ':', "part", "A:part:B"); // both sides include the separator
}
