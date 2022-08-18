// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "tests_common.h"

#include <iostream>

#include <boost/program_options.hpp>

#include "test_data.h"

#if defined(DO_INTERFACE_COM)
#include <combaseapi.h>
#endif

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);

    namespace po = boost::program_options;
    po::options_description desc("Options");
    desc.add_options()
        ("help,h", "Show help messages")
        ("mcc-host,m", po::value<std::string>(), "MCC hostname to use (optional override)")
        ("manual-start", po::bool_switch()->default_value(false), "Wait for command line input to start tests (useful for debugging)");
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);

    auto it = vm.find("mcc-host");
    if (it != vm.end())
    {
        g_mccHostName = it->second.as<std::string>();
        std::cout << "Got overriden MCC host: " << g_mccHostName << '\n';
    }

#if defined(DO_INTERFACE_COM)
    // SDK leaves com init up to caller, so initialize in test exe here
    const auto hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        std::cout << "CoInitializeEx failed with " << hr << std::endl;
        return hr;
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

    int testsResult = RUN_ALL_TESTS();

#if defined(DO_INTERFACE_COM)
    CoUninitialize();
#endif

    return testsResult;
}
