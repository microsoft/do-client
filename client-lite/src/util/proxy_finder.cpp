// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "do_common.h"
#include "proxy_finder.h"

#ifdef DO_PROXY_SUPPORT
#include <proxy.h> // libproxy
#endif

static char** GetProxiesForURL(const char* url)
{
    char** proxies = nullptr;
#ifdef DO_PROXY_SUPPORT
    auto proxyFactory = px_proxy_factory_new();
    if (proxyFactory != nullptr)
    {
        proxies = px_proxy_factory_get_proxies(proxyFactory, url);
        px_proxy_factory_free(proxyFactory);
    }
#endif
    return proxies;
}

ProxyFinder::ProxyFinder(PCSTR url)
{
    _proxyList = GetProxiesForURL(url);
}

ProxyFinder::~ProxyFinder()
{
    if (_proxyList != nullptr)
    {
        for (size_t i = 0; _proxyList[i]; ++i)
        {
            free(_proxyList[i]);
        }
        free(_proxyList);
    }
}

ProxyFinder::proxy_list_t ProxyFinder::Get() const
{
    proxy_list_t result;
    if (_proxyList != nullptr)
    {
        for (size_t i = 0; _proxyList[i]; ++i)
        {
            char* cur = _proxyList[i];
            DoLogDebug("Proxy[%zu]: %s", i, cur);
            // direct is used to denote 'no proxy'
            if (strcmp(cur, "direct://") != 0)
            {
                result.emplace_back(cur);
            }
        }
    }
    return result;
}

void ProxyList::Refresh(const std::string& url)
{
    _candidateProxies = ProxyFinder(url.data()).Get();
    Reset();
}

// Iterate through the list in round-robin fashion
const ProxyList::proxy_value_t& ProxyList::Next()
{
    static proxy_value_t empty;
    if (Empty())
    {
        return empty;
    }
    const auto& ret = *_it++;
    if (_it == _candidateProxies.cend())
    {
        Reset();
    }
    return ret;
}
