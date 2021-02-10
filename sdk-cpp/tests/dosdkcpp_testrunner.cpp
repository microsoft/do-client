#include "tests_common.h"

#include <boost/program_options.hpp>
#include "test_data.h"

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);

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
