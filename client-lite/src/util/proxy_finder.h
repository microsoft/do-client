// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <string>
#include <vector>

class ProxyFinder
{
public:
    using proxy_list_t = std::vector<std::string>;

    ProxyFinder(PCSTR url);
    ~ProxyFinder();

    proxy_list_t Get() const;

private:
    char** _proxyList { nullptr };
};

class ProxyList
{
public:
    using proxy_value_t = ProxyFinder::proxy_list_t::value_type;

    ProxyList()
    {
        Reset();
    }

    void Refresh(const std::string& url);

    const proxy_value_t& Next();

    void Reset()
    {
        _it = _candidateProxies.cbegin();
    }

    size_t Size() const
    {
        return _candidateProxies.size();
    }

    bool Empty() const
    {
        return (Size() == 0);
    }

private:
    ProxyFinder::proxy_list_t _candidateProxies;
    ProxyFinder::proxy_list_t::const_iterator _it;
};
