#include "test_common.h"
#include "rest_http_listener.h"

#include <thread>
#include <boost/asio.hpp>

using btcp_t = boost::asio::ip::tcp;
using bsock_t = boost::asio::ip::tcp::socket;

class BoostAsioWorker
{
public:
    BoostAsioWorker() :
        _work(_io)
    {
        _myThread = std::thread([this]()
        {
            _io.run();
        });
    }

    boost::asio::io_service& IoService() { return _io; }

    ~BoostAsioWorker()
    {
        _io.stop();
        _myThread.join();
    }

private:
    boost::asio::io_service _io;
    boost::asio::io_service::work _work;
    std::thread _myThread;
};

TEST(RestListenerTests, ManyPortsInUse)
{
    // Listener searches for ports from 50000 to 60999.
    // We try to block all ports until 54999 and check time taken for RestHttpListener::Start().

    BoostAsioWorker asioWorker;

    // Create and bind as many sockets as possible (usual limit is 1024 total open file descriptors)
    std::vector<bsock_t> sockets;
    UINT portsAlreadyInUse = 0;
    for (UINT port = 50000; port <= 54999; ++port)
    {
        try
        {
            bsock_t sock(asioWorker.IoService());
            sock.open(btcp_t::v4());
            sock.bind(btcp_t::endpoint(btcp_t::v4(), port));
            sockets.push_back(std::move(sock));
        }
        catch (const boost::system::system_error& e)
        {
            if (e.code() == boost::system::errc::address_in_use)
            {
                ++portsAlreadyInUse;
                continue;
            }

            if (e.code() == boost::system::errc::too_many_files_open)
            {
                std::cout << "Caught exception: " << e.what() << ". Iteration " << port - 50000 + 1 << " now.\n";
                // Free up some sockets to allow REST listener to start
                for (UINT c = 1; c <= 10; ++c)
                {
                    if (sockets.empty())
                    {
                        break;
                    }
                    sockets.pop_back();
                }
                break;
            }

            throw;
        }
    }

    std::cout << "Total number of ports already in use: " << portsAlreadyInUse << "\n";
    std::cout << "Total number of sockets open: " << sockets.size() << "\n";

    RestHttpListener listener;
    const auto before = std::chrono::steady_clock::now();
    listener.Start(asioWorker.IoService(), http_listener_callback_t{});
    const auto after = std::chrono::steady_clock::now();
    std::cout << "Listener started at: " << listener.Endpoint() << "\n";
    const auto elapsedMsecs = std::chrono::duration_cast<std::chrono::milliseconds>(after - before).count();
    std::cout << "Time taken for listener to start: " << elapsedMsecs << "ms\n";
    EXPECT_LT(elapsedMsecs, 2000);
}
