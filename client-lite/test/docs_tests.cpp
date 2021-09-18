#include "test_common.h"

#include <boost/program_options.hpp>
#include "test_data.h"

const cppfs::path g_testTempDir = "/tmp/docs_test_scratch";

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    // DoTraceLoggingRegister(); // enable failure output to console
    // TODO(shishirb) enable console only logging

    std::error_code ec;
    cppfs::create_directories(g_testTempDir, ec);
    if (ec)
    {
        printf("Failed to create test dir: %s\n", g_testTempDir.string().data());
        return ec.value();
    }

    namespace po = boost::program_options;
    po::options_description desc("Options");
    desc.add_options()
        ("help,h", "Show help messages")
        ("mcc-host,m", po::value<std::string>(), "MCC hostname to use (optional override)");
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    auto it = vm.find("mcc-host");
    if (it != vm.end())
    {
        g_mccHostName = it->second.as<std::string>();
        std::cout << "Got overriden MCC host: " << g_mccHostName << '\n';
    }

    return RUN_ALL_TESTS();
}
