#include "test_common.h"
#include "test_helpers.h"

#include "do_test_helpers.h"
namespace dtu = dotest::util;

void TestHelpers::DisableNetwork()
{
    dtu::ExecuteSystemCommand("ifconfig eth0 down");
    std::cout << "Disabled eth0" << '\n';
}

void TestHelpers::EnableNetwork()
{
    dtu::ExecuteSystemCommand("ifconfig eth0 up");
    std::cout << "Enabled eth0" << '\n';
}
