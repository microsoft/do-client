#pragma once

#include <mutex>
#include <boost/property_tree/ptree.hpp>
#include "do_noncopyable.h"

namespace microsoft::deliveryoptimization::details
{
class CHttpClientImpl;

class CHttpClient : CDONoncopyable
{
public:
    enum Method
    {
        GET,
        POST
    };

    ~CHttpClient();
    static CHttpClient& GetInstance();
    boost::property_tree::ptree SendRequest(Method method, const std::string& url, bool retry = true);

private:
    CHttpClient();
    void _InitializeDOConnection(bool launchClientFirst = false);

    mutable std::mutex _mutex;
    std::unique_ptr<CHttpClientImpl> _httpClientImpl;
};

} // namespace microsoft::deliveryoptimization::details
