// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "tests_common.h"

#include <cstring>
#include <experimental/filesystem>
#include "do_config.h"
#include "do_persistence.h"
#include "test_data.h"
#include "test_helpers.h"

namespace cppfs = std::experimental::filesystem;
namespace msdo = microsoft::deliveryoptimization;
namespace msdod = microsoft::deliveryoptimization::details;

TEST(ConfigVersionTests, GetVersion)
{
    char* version = deliveryoptimization_get_components_version();
    ASSERT_NE(*version, '\0');
    ASSERT_EQ(strchr(version, '\n'), nullptr);
    std::cout << "All versions: " << version << std::endl;
    ASSERT_EQ(strstr(version, "DU;lib"), version); // <BUILDER NAME>;<component name> found at beginning
    ASSERT_EQ(strstr(version, "deliveryoptimization-"), nullptr); // prefix not found in any component
    deliveryoptimization_free_version_buf(&version);
    ASSERT_EQ(version, nullptr);
}
