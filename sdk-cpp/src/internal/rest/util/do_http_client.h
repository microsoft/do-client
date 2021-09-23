#ifndef _DELIVERY_OPTIMIZATION_DO_HTTP_CLIENT_H
#define _DELIVERY_OPTIMIZATION_DO_HTTP_CLIENT_H

#include <mutex>
#include <boost/property_tree/ptree.hpp>
#include "do_http_message.h"
#include "do_noncopyable.h"

namespace microsoft
{
namespace deliveryoptimization
{
namespace details
{

class CHttpClientImpl;

class CHttpClient : CDONoncopyable
{
public:
    ~CHttpClient();
    static CHttpClient& GetInstance();
    int32_t SendRequest(boost::property_tree::ptree& tree, HttpRequest::Method method, const std::string& url, bool retry = true);

private:
    CHttpClient();
    int32_t _InitializeDOConnection(bool launchClientFirst = false);

    mutable std::mutex _mutex;
    std::unique_ptr<CHttpClientImpl> _httpClientImpl;
};
} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft
#endif
