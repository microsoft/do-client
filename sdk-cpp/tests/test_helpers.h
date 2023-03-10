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
#include "do_test_helpers.h"

namespace msdo = microsoft::deliveryoptimization;
namespace msdod = microsoft::deliveryoptimization::details;

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
    static std::unique_ptr<download> make(const std::string& uri)
    {
        std::unique_ptr<msdo::download> newDownload;
        std::error_code ec = msdo::download::make(uri, newDownload);
        msdod::throw_if_fail(ec);
        auto returnVal = std::make_unique<download>(std::move(newDownload));
        return returnVal;
    }

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
    void set_status_callback(status_callback_t callback)
    {
        std::error_code ec = _downloadImpl->set_status_callback(callback);
        msdod::throw_if_fail(ec);
    }
    void set_output_stream(output_stream_callback_t callback)
    {
        std::error_code ec = _downloadImpl->set_output_stream(callback);
        msdod::throw_if_fail(ec);
    }
    void set_ranges(const download_range* ranges, size_t count)
    {
        std::error_code ec = _downloadImpl->set_ranges(ranges, count);
        msdod::throw_if_fail(ec);
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

    template <typename T>
    void set_property(msdo::download_property key, const T& value)
    {
        std::error_code ec = _downloadImpl->set_property(key, value);
        msdod::throw_if_fail(ec);
    }
    template <typename T>
    void get_property(msdo::download_property key, T& value)
    {
        std::error_code ec = _downloadImpl->get_property(key, value);
        msdod::throw_if_fail(ec);
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

    // DoSvc creates temporary files with a unique name in the same directory as the output file
    static void DeleteDoSvcTemporaryFiles(const boost::filesystem::path& outputFilePath)
    {
        const boost::filesystem::path parentDir = outputFilePath.parent_path();
        for (boost::filesystem::directory_iterator itr(parentDir); itr != boost::filesystem::directory_iterator(); ++itr)
        {
            const boost::filesystem::directory_entry& dirEntry = *itr;
            // Remove all files with names that match DO*.tmp
            if (boost::filesystem::is_regular_file(dirEntry)
                && (dirEntry.path().filename().string().find("DO") == 0)
                && (dirEntry.path().extension() == ".tmp"))
            {
                boost::system::error_code ec;
                boost::filesystem::remove(dirEntry, ec);
                if (ec)
                {
                    std::cout << "Temp file deletion error: " << ec.message() << ", " << dirEntry.path() << '\n';
                }
                else
                {
                    std::cout << "Deleted DoSvc temp file: " << dirEntry.path() << '\n';
                }
            }
        }
    }

    // On Windows, operations are async - there may be some delay setting a state internally
    static void WaitForState(microsoft::deliveryoptimization::test::download& download, msdo::download_state expectedState,
        std::chrono::seconds waitTimeSecs = std::chrono::seconds{10})
    {
        msdo::download_status status = download.get_status();
        const auto endtime = std::chrono::steady_clock::now() + waitTimeSecs;
        while ((status.state() != expectedState) && (std::chrono::steady_clock::now() < endtime))
        {
            std::this_thread::sleep_for(std::chrono::seconds{1});
            status = download.get_status();
            std::cout << "Transferred " << status.bytes_transferred() << " / " << status.bytes_total() << "\n";
        }

        if (status.state() != expectedState)
        {
            // Throw exception instead of ASSERT* to let tests catch if needed
            const auto msg = dotest::util::FormatString("State: expected = %d, actual = %d", expectedState, status.state());
            throw std::runtime_error(msg);
        }
    }

#ifdef DO_INTERFACE_REST
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
    // Disallow creating an instance of this object
    TestHelpers() {}

};

#endif // _DELIVERY_OPTIMIZATION_TEST_HELPERS_H
