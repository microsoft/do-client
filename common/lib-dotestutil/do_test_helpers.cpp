// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "do_test_helpers.h"

#include <future>
#include <iostream>

namespace dotest
{
namespace util
{

void ExecuteSystemCommand(const char* command)
{
#ifdef DO_PLATFORM_WINDOWS
    throw std::exception("System command not implemented");
#else
    int ret = system(command);
    if (!WIFEXITED(ret))
    {
        throw DOTestException("Command [%s] did not exit normally, ret: %d", command, ret);
    }

    int cmdRet = WEXITSTATUS(ret);
    if (cmdRet != 0)
    {
        throw DOTestException("Command [%s] completed with error: %d", command, cmdRet);
    }
#endif
}

#ifdef DO_PLATFORM_LINUX
using btcp_t = boost::asio::ip::tcp;

BoostAsioWorker::~BoostAsioWorker()
{
    _io.stop();
    _myThread.join();
}

// Returns nullptr if no address was found for the DNS query
std::unique_ptr<btcp_t::endpoint> BoostAsioWorker::ResolveDnsQuery(const btcp_t::resolver::query& resolverQuery,
    const boost::asio::ip::tcp::resolver::protocol_type* prot)
{
    std::promise<std::unique_ptr<btcp_t::endpoint>> epPromise;
    auto fut = epPromise.get_future();
    auto fnResolveHandler = [&epPromise, prot](const boost::system::error_code& ec, btcp_t::resolver::iterator endpoints) -> void
    {
        std::unique_ptr<btcp_t::endpoint> spFoundEp;
        if (ec)
        {
            std::cout << FormatString("Error resolving address: %d, %s\n", ec.value(), ec.message().c_str());
        }
        else if (endpoints == btcp_t::resolver::iterator())
        {
            std::cout << "Failed to resolve address to any endpoints\n";
        }
        else
        {
            std::cout << "Resolved endpoints:\n";
            while (endpoints != btcp_t::resolver::iterator())
            {
                const auto& ep = *endpoints++;
                std::cout << FormatString("Host: %s, IP: %s\n", ep.host_name().data(), ep.endpoint().address().to_string().c_str());
                if ((prot == nullptr) || (ep.endpoint().protocol() == *prot))
                {
                    spFoundEp = std::make_unique<btcp_t::endpoint>(ep);
                }
            }
        }
        if (spFoundEp)
        {
            std::cout << FormatString("Returning IP: %s\n", spFoundEp->address().to_string().c_str());
        }
        epPromise.set_value(std::move(spFoundEp));
    };

    btcp_t::resolver queryResolver(_io);
    std::cout << "Issuing query: " << resolverQuery.host_name() << ":" << resolverQuery.service_name() << '\n';
    queryResolver.async_resolve(resolverQuery, fnResolveHandler);
    std::cout << "Waiting...\n";
    if (fut.wait_for(std::chrono::seconds(30)) == std::future_status::timeout)
    {
        std::cout << "Timed out after 30s\n";
        return {};
    }
    return fut.get();
}
#endif // DO_PLATFORM_LINUX

} // namespace util
} // namespace dotest
