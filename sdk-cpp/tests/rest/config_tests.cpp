// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "tests_common.h"

#include <cstring>
#include <filesystem>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "do_config.h"
#include "do_persistence.h"
#include "test_data.h"
#include "test_helpers.h"

namespace msdo = microsoft::deliveryoptimization;
namespace msdod = microsoft::deliveryoptimization::details;

class ConfigTests : public ::testing::Test
{
public:
    void SetUp() override
    {
        TestHelpers::CleanTestDir();
        if (std::filesystem::exists(msdod::GetConfigFilePath()))
        {
            std::filesystem::remove(msdod::GetConfigFilePath());
        }
    }

    void TearDown() override
    {
        SetUp();
        // Make sure docs reloads config immediately
        TestHelpers::RestartService(g_docsSvcName);
    }
};

TEST_F(ConfigTests, IoTConnectionString)
{
    const char* expectedValue = "HostName=instance-company-iothub-ver.host.tld;DeviceId=user-dev-name;SharedAccessKey=abcdefghijklmnopqrstuvwxyzABCDE123456789012=;GatewayHostName=10.0.0.200";
    int ret = deliveryoptimization_set_iot_connection_string(expectedValue);
    ASSERT_EQ(ret, 0);

    boost::property_tree::ptree configTree;
    boost::property_tree::read_json(msdod::GetConfigFilePath(), configTree);

    const std::string value = configTree.get<std::string>("ADUC_IoTConnectionString");
    ASSERT_EQ(value, std::string{expectedValue});
}

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
