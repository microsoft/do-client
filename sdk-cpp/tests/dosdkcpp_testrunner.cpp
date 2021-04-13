#include "tests_common.h"

#include <iostream>

#include <boost/program_options.hpp>

#if (DO_PLATFORM_ID == DO_PLATFORM_ID_LINUX)
#include "test_data.h"
#endif

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);

    namespace po = boost::program_options;
    po::options_description desc("Options");
    desc.add_options()
        ("help,h", "Show help messages")
#if (DO_PLATFORM_ID == DO_PLATFORM_ID_LINUX)
        ("mcc-host,m", po::value<std::string>(), "MCC hostname to use (optional override)")
#endif
        ("manual-start", po::bool_switch()->default_value(false), "Wait for command line input to start tests (useful for debugging)");
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);

#if (DO_PLATFORM_ID == DO_PLATFORM_ID_LINUX)
    auto it = vm.find("mcc-host");
    if (it != vm.end())
    {
        g_mccHostName = it->second.as<std::string>();
        std::cout << "Got overriden MCC host: " << g_mccHostName << '\n';
    }
#endif 

    auto manualStart = vm["manual-start"].as<bool>();
    if (manualStart)
    {
        do
        {
            std::cout << '\n' << "Press a key to continue...";
        } while (std::cin.get() != '\n');
    }

    return RUN_ALL_TESTS();
}
