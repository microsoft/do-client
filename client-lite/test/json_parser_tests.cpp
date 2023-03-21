// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "test_common.h"
#include "do_json_parser.h"

#include <fstream>
#include <iostream>
#include <thread>
#include <string>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

const std::filesystem::path g_jsonTestFilePath = g_testTempDir / "docs_config.json";

const std::map<std::string, std::string> g_testData =
{
    { "key1", "value1" },
    { "DOCacheHost", "10.0.0.100:80,myhost2.com:8080" },
    { "IoTConnectionString", "HostName=instance-company-iothub-ver.host.tld;DeviceId=user-dev-name;SharedAccessKey=abcdefghijklmnopqrstuvwxyzABCDE123456789012=;GatewayHostName=10.0.0.200" }
};

static void WriteOutTestData()
{
    boost::property_tree::ptree configTree;

    for (const auto& item : g_testData)
    {
        configTree.put(item.first, item.second);
    }

    boost::property_tree::write_json(g_jsonTestFilePath.string(), configTree);
}

static void VerifyItemFound(JsonParser& reader, const std::string& key, const std::string& expectedValue)
{
    boost::optional<std::string> configValue = reader.Get<std::string>(key);
    std::string stringValue = boost::get_optional_value_or(configValue, std::string{});
    ASSERT_TRUE(!stringValue.empty());
    ASSERT_EQ(expectedValue, stringValue);
    std::cout << "Retrieved config: " << key << " = " << stringValue.data() << '\n';
}

static void VerifyItemNotFound(JsonParser& reader, const std::string& key)
{
    boost::optional<std::string> configValue = reader.Get<std::string>(key);
    std::string stringValue = boost::get_optional_value_or(configValue, std::string{});
    ASSERT_TRUE(stringValue.empty());
}

static void VerifyTestData(JsonParser& reader, bool fItemsFound = true)
{
    for (const auto& item : g_testData)
    {
        fItemsFound ? VerifyItemFound(reader, item.first, item.second) : VerifyItemNotFound(reader, item.first);
    }
}

static JsonParser VerifyTestData()
{
    JsonParser jsonReader(g_jsonTestFilePath.string());
    VerifyTestData(jsonReader);
    return jsonReader;
}

class JsonParserTests : public ::testing::Test
{
public:
    void SetUp() override
    {
        JsonParser::RefreshInterval = std::chrono::seconds(10); // for faster test times
        if (std::filesystem::exists(g_jsonTestFilePath))
        {
            std::filesystem::remove(g_jsonTestFilePath);
        }
    }
};

TEST_F(JsonParserTests, ReadConfigs)
{
    WriteOutTestData();
    JsonParser reader = VerifyTestData();

    // Reading non-existent results in empty return, no exceptions
    VerifyItemNotFound(reader, "nonexistentkey");
}

TEST_F(JsonParserTests, ReadNonExistentConfigFile)
{
    // Reading non-existent file results in empty return when querying for configs, no exceptions
    JsonParser reader("/tmp/do-non-existent-config.json");
    VerifyItemNotFound(reader, "nonexistentkey");
}

TEST_F(JsonParserTests, ReadConfigsWithUpdates)
{
    WriteOutTestData();
    JsonParser reader = VerifyTestData();

    boost::property_tree::ptree configTree;
    boost::property_tree::read_json(g_jsonTestFilePath, configTree);
    configTree.put("newkey", "newvalue");
    boost::property_tree::write_json(g_jsonTestFilePath.string(), configTree);

    // Item not found yet because refresh interval hasn't passed
    VerifyItemNotFound(reader, "newkey");

    // Find succeeds after the refresh interval elapses
    std::this_thread::sleep_for(JsonParser::RefreshInterval);
    VerifyItemFound(reader, "newkey", "newvalue");

    // Delete config file and verify
    std::filesystem::remove(g_jsonTestFilePath);
    VerifyItemFound(reader, "newkey", "newvalue"); // within refresh interval
    VerifyTestData(reader, true);

    std::this_thread::sleep_for(JsonParser::RefreshInterval);
    VerifyItemNotFound(reader, "newkey"); // after refresh interval
    VerifyTestData(reader, false);
}

TEST_F(JsonParserTests, ReadConfigsWithFileCreatedLater)
{
    if (std::filesystem::exists(g_jsonTestFilePath))
    {
        std::filesystem::remove(g_jsonTestFilePath);
    }

    JsonParser reader(g_jsonTestFilePath);
    VerifyItemNotFound(reader, g_testData.begin()->first);

    WriteOutTestData();

    // Item still not found because refresh interval hasn't passed
    VerifyItemNotFound(reader, g_testData.begin()->first);

    // Find succeeds after the refresh interval elapses
    std::this_thread::sleep_for(JsonParser::RefreshInterval);
    VerifyTestData();
}
