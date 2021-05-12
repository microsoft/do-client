#include "tests_common.h"

#include <fstream>

#include <boost/filesystem.hpp>

#include "do_persistence.h"
#include "do_port_finder.h"

namespace msdod = microsoft::deliveryoptimization::details;

static const char* const samplePortNumber = "50000";

class PortDiscoveryTests : public ::testing::Test
{
public:
    PortDiscoveryTests()
    {
        _testFilePath = std::string(msdod::GetRuntimeDirectory()) + std::string("/restport.0");
    }

    void SetUp() override;
    void TearDown() override;

    void DiscoverPortTest();
    void DiscoverUrlTest();

private:
    std::string _testFilePath;
};

void PortDiscoveryTests::SetUp()
{
    if (boost::filesystem::exists(_testFilePath))
    {
        boost::filesystem::remove(_testFilePath);
    }
    if (!boost::filesystem::exists(msdod::GetRuntimeDirectory()))
    {
        boost::filesystem::create_directories(msdod::GetRuntimeDirectory());
    }

    std::ofstream file(_testFilePath);
    file << samplePortNumber << std::endl;
}

void PortDiscoveryTests::TearDown()
{
    if (boost::filesystem::exists(_testFilePath))
    {
        boost::filesystem::remove(_testFilePath);
    }
}

TEST_F(PortDiscoveryTests, DiscoverUrlTest)
{
    std::string url = msdod::CPortFinder::GetDOPort(false);
    ASSERT_EQ(url, samplePortNumber);
}
