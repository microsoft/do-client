// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _DELIVERY_OPTIMIZATION_TEST_HELPERS_H
#define _DELIVERY_OPTIMIZATION_TEST_HELPERS_H

#include <string>
#include <thread>

#include "tests_common.h"
#include "test_data.h"
#include "do_download.h"
#include "do_download_status.h"
#include "do_error_helpers.h"

namespace msdo = microsoft::deliveryoptimization;
namespace msdod = microsoft::deliveryoptimization::details;
using namespace std::chrono_literals; // NOLINT(build/namespaces)

namespace microsoft
{
namespace deliveryoptimization
{
namespace test
{

// Shim class to convert error-return API of the SDK download class to exception-throwing API.
// Avoids the need to rewrite tests to check the return code of each API.
class download
{
public:
    static std::unique_ptr<download> make(const std::string& uri, const std::string& downloadFilePath)
    {
        std::unique_ptr<msdo::download> newDownload;
        std::error_code ec = msdo::download::make(uri, downloadFilePath, newDownload);
        msdod::throw_if_fail(ec);
        auto returnVal = std::make_unique<download>(std::move(newDownload));
        return returnVal;
    }

    download(std::unique_ptr<msdo::download>&& downloadImpl) :
        _downloadImpl(std::move(downloadImpl))
    {
    }

    void start()
    {
        std::error_code ec = _downloadImpl->start();
        msdod::throw_if_fail(ec);
    }
    void pause()
    {
        std::error_code ec = _downloadImpl->pause();
        msdod::throw_if_fail(ec);
    }
    void resume()
    {
        std::error_code ec = _downloadImpl->resume();
        msdod::throw_if_fail(ec);
    }
    void finalize()
    {
        std::error_code ec = _downloadImpl->finalize();
        msdod::throw_if_fail(ec);
    }
    void abort()
    {
        std::error_code ec = _downloadImpl->abort();
        msdod::throw_if_fail(ec);
    }
    msdo::download_status get_status() const
    {
        msdo::download_status status;
        std::error_code ec = _downloadImpl->get_status(status);
        msdod::throw_if_fail(ec);
        return status;
    }

    void start_and_wait_until_completion(std::chrono::seconds timeoutSecs = std::chrono::hours(24))
    {
        std::error_code ec = _downloadImpl->start_and_wait_until_completion(timeoutSecs);
        msdod::throw_if_fail(ec);
    }
    void start_and_wait_until_completion(const std::atomic_bool& isCancelled, std::chrono::seconds timeoutSecs = std::chrono::hours(24))
    {
        std::error_code ec = _downloadImpl->start_and_wait_until_completion(isCancelled, timeoutSecs);
        msdod::throw_if_fail(ec);
    }

    static void download_url_to_path(const std::string& uri, const std::string& downloadFilePath,
        std::chrono::seconds timeoutSecs = std::chrono::hours(24))
    {
        std::error_code ec = msdo::download::download_url_to_path(uri, downloadFilePath, timeoutSecs);
        msdod::throw_if_fail(ec);
    }
    static void download_url_to_path(const std::string& uri, const std::string& downloadFilePath, const std::atomic_bool& isCancelled,
        std::chrono::seconds timeoutSecs = std::chrono::hours(24))
    {
        std::error_code ec = msdo::download::download_url_to_path(uri, downloadFilePath, isCancelled, timeoutSecs);
        msdod::throw_if_fail(ec);
    }

    /*
    For devices running windows before 20H1, dosvc exposed a now-deprecated com interface for setting certain download properties.
    After 20H1, these properties were added to newer com interface, which this SDK is using.
    Attempting to set a download property on a version of windows earlier than 20H1 will not set the property and throw an exception with error code msdo::errc::do_e_unknown_property_id
    */
    void set_property(msdo::download_property key, const msdo::download_property_value& value)
    {
        std::error_code ec = _downloadImpl->set_property(key, value);
        msdod::throw_if_fail(ec);
    }
    msdo::download_property_value get_property(msdo::download_property key)
    {
        msdo::download_property_value propValue;
        std::error_code ec = _downloadImpl->get_property(key, propValue);
        msdod::throw_if_fail(ec);
        return propValue;
    }

private:
    std::unique_ptr<msdo::download> _downloadImpl;
};

}
}
}

class TestHelpers
{
public:
    static void CleanTestDir()
    {
        if (boost::filesystem::exists(g_tmpFileName))
        {
            boost::filesystem::remove(g_tmpFileName);
        }
        if (boost::filesystem::exists(g_tmpFileName2))
        {
            boost::filesystem::remove(g_tmpFileName2);
        }
        if (boost::filesystem::exists(g_tmpFileName3))
        {
            boost::filesystem::remove(g_tmpFileName3);
        }
    }

    // On Windows, operations are async - there may be some delay setting a state internally
    static void WaitForState(microsoft::deliveryoptimization::test::download& download, msdo::download_state expectedState,
        std::chrono::seconds waitTimeSecs = 10s)
    {
        msdo::download_status status = download.get_status();
        const auto endtime = std::chrono::steady_clock::now() + waitTimeSecs;
        while ((status.state() != expectedState) && (std::chrono::steady_clock::now() < endtime))
        {
            std::this_thread::sleep_for(1s);
            status = download.get_status();
            std::cout << "Transferred " << status.bytes_transferred() << " / " << status.bytes_total() << "\n";
        }

        ASSERT_EQ(status.state(), expectedState) << "Download must have reached expected state before timeout";
    }

#ifdef DO_INTERFACE_REST
    static bool IsActiveProcess(std::string name);
    static int ShutdownProcess(std::string name);
    static void RestartService(const std::string& name);
    static void StartService(const std::string& name);
    static void StopService(const std::string& name);
    static void CreateRestPortFiles(int numFiles);
    static void DeleteRestPortFiles();
    static unsigned int CountRestPortFiles();
    static void DisableNetwork();
    static void EnableNetwork();

    static std::string GetLocalIPv4Address();
#endif // DO_INTERFACE_REST

private:
#ifdef DO_INTERFACE_REST
    static int _GetPidFromProcName(std::string name);
    static int _KillProcess(int pid, int signal);
#endif // DO_INTERFACE_REST

    // Disallow creating an instance of this object
    TestHelpers() {}

};

#endif // _DELIVERY_OPTIMIZATION_TEST_HELPERS_H
