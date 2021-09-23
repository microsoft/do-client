#include "tests_common.h"

#include <fstream>

#include <boost/filesystem.hpp>

#include "do_persistence.h"
#include "do_port_finder.h"
#include "test_data.h"
#include "test_helpers.h"

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

private:
    std::string _testFilePath;
};

void PortDiscoveryTests::SetUp()
{
    TestHelpers::StopService(g_docsSvcName); // ensure agent does not write to port file
    TestHelpers::DeleteRestPortFiles();
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
    TestHelpers::StartService(g_docsSvcName);
}

TEST_F(PortDiscoveryTests, DiscoverPortTest)
{
    std::string foundPort;
    int32_t res = msdod::CPortFinder::GetDOPort(foundPort, false);
    ASSERT_EQ(res, errc::s_ok);
    ASSERT_EQ(foundPort, samplePortNumber);
}
