// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "tests_common.h"

#include <fstream>
#include <experimental/filesystem>
namespace cppfs = std::experimental::filesystem;

#include <boost/asio.hpp>
using btcp_t = boost::asio::ip::tcp;

#include "do_persistence.h"
namespace msdod = microsoft::deliveryoptimization::details;

#include "do_port_finder.h"
#include "do_test_helpers.h"
#include "test_data.h"
#include "test_helpers.h"

class RestInterfaceTests : public ::testing::Test
{
public:
    void SetUp() override
    {
        if (cppfs::exists(msdod::GetAdminConfigFilePath()))
        {
            cppfs::remove(msdod::GetAdminConfigFilePath());
        }
    }

    void TearDown() override
    {
        SetUp();
        // Make sure docs reloads config immediately
        TestHelpers::RestartService(g_docsSvcName);
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
};

void VerifyRestInterfaceWithLocalEndpoint(const btcp_t::endpoint& localEndpoint, const std::string& expectedReponseSubString,
    dotest::util::BoostAsioWorker& asioService)
{
    std::cout << "Will bind to local address: " << localEndpoint << std::endl;

    auto sock = btcp_t::socket(asioService.Service(), btcp_t::v4());
    boost::system::error_code ec;
    sock.bind(localEndpoint, ec);
    ASSERT_FALSE(ec) << "Expect no bind failure but got: " << ec.message();

    auto addr = boost::asio::ip::address::from_string("127.0.0.1");

    const auto restPortStr = microsoft::deliveryoptimization::details::CPortFinder::GetDOPort();
    ASSERT_TRUE(!restPortStr.empty());

    const auto restPort = static_cast<unsigned short>(std::stoul(restPortStr));
    std::cout << "Using rest port: " << restPort << std::endl;
    auto remoteEp = btcp_t::endpoint(addr, restPort);
    sock.connect(remoteEp, ec);
    ASSERT_FALSE(ec) << "Expect no connect failure but got: " << ec.message();

    std::cout << "Connected socket local endpoint: " << sock.local_endpoint() << std::endl;
    std::cout << "Connected socket remote endpoint: " << sock.remote_endpoint() << std::endl;

    const std::string buf = "POST /download/create/ HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n";
    auto sendBuf = boost::asio::buffer(buf.data(), buf.size());
    sock.send(sendBuf, 0, ec);
    ASSERT_FALSE(ec) << "Expect no send failure but got: " << ec.message();

    std::vector<char> incoming(8192);
    auto cbRead = sock.read_some(boost::asio::buffer(incoming, incoming.size()), ec);
    std::cout << "Error? " << bool{ec} << ", msg: " << ec.message() << ", cbRead: " << cbRead << std::endl;

    std::string responseStr(incoming.data(), incoming.data() + cbRead);
    std::cout << "Response: " << responseStr.data() << std::endl;

    ASSERT_NE(responseStr.find(expectedReponseSubString), std::string::npos) << "Expected substr in response: " << expectedReponseSubString;
}

TEST_F(RestInterfaceTests, RestInterfaceUseLocalHostForLocalSocket)
{
    dotest::util::BoostAsioWorker asioService;

    const auto localHostname = boost::asio::ip::host_name();
    btcp_t::resolver::query query(localHostname, "");
    auto spLocalEp = asioService.ResolveDnsQuery(query);
    ASSERT_TRUE(spLocalEp) << "Found at least one address for the local hostname query";

    // Hostname can resolve to either a loopback address or private address depending on machine/network config
    const auto expectedStr = spLocalEp->address().is_loopback() ? "200 OK" : "400 BadRequest";
    VerifyRestInterfaceWithLocalEndpoint(*spLocalEp, expectedStr, asioService);
}

// Explicit loopback address for local endpoint
TEST_F(RestInterfaceTests, RestInterfaceUseLoopbackForLocalSocket)
{
    dotest::util::BoostAsioWorker asioService;

    auto loopbackIpAddr = boost::asio::ip::address::from_string("127.0.1.5");
    auto loopbackEp = btcp_t::endpoint(loopbackIpAddr, 0);
    VerifyRestInterfaceWithLocalEndpoint(loopbackEp, "200 OK", asioService);
}

// Explicit non-loopback address for local endpoint
TEST_F(RestInterfaceTests, RestInterfaceUsePrivateIPForLocalSocket)
{
    dotest::util::BoostAsioWorker asioService;

    auto privateIpAddr = boost::asio::ip::address::from_string(TestHelpers::GetLocalIPv4Address());
    ASSERT_TRUE(!privateIpAddr.is_loopback());
    auto privateEp = btcp_t::endpoint(privateIpAddr, 0);
    VerifyRestInterfaceWithLocalEndpoint(privateEp, "400 BadRequest", asioService);

    // With the validation turned off, verify that request succeeds
    std::ofstream writer;
    writer.exceptions(std::fstream::badbit | std::fstream::failbit);
    writer.open(msdod::GetAdminConfigFilePath(), std::ios_base::out | std::ios_base::trunc);
    writer << "{ \"RestControllerValidateRemoteAddr\":false }" <<  '\n';
    writer.flush();

    TestHelpers::RestartService(g_docsSvcName); // force reload config file instead of waiting for refresh interval
    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::cout << "Testing again after disabling remote address validation\n";
    VerifyRestInterfaceWithLocalEndpoint(privateEp, "200 OK", asioService);
}
