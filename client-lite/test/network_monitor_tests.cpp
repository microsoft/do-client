#include "test_common.h"

#include <thread>
#include <chrono>

#include "network_monitor.h"
#include "test_helpers.h"

using namespace std::chrono_literals; // NOLINT(build/namespaces)

class NetworkMonitorTests : public ::testing::Test
{
public:
    void SetUp() override;
    void TearDown() override;

    void _EnableNetwork();
    void _DisableNetwork();
};

void NetworkMonitorTests::SetUp()
{
    _EnableNetwork();
}

void NetworkMonitorTests::TearDown()
{
    _EnableNetwork();
}

// This causes build pipeline failure due to cutting off communication between the agent and backend pipeline infra.
// We can remove this test once we have it running as part of the E2E test suite.
TEST_F(NetworkMonitorTests, DISABLED_VerifyNetworkReconnect)
{
    ASSERT_TRUE(NetworkMonitor::IsConnected());

    _DisableNetwork();
    ASSERT_FALSE(NetworkMonitor::IsConnected());

    _EnableNetwork();
    ASSERT_TRUE(NetworkMonitor::IsConnected());
}

void NetworkMonitorTests::_EnableNetwork()
{
    TestHelpers::EnableNetwork();
    std::this_thread::sleep_for(10s);
}

void NetworkMonitorTests::_DisableNetwork()
{
    TestHelpers::DisableNetwork();
    std::this_thread::sleep_for(10s);
}
