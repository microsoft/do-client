// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "do_common.h"
#include "rest_api_parser.h"

#include "do_http_defines.h"
#include "do_cpprest_uri.h"
#include "string_ops.h"

namespace msdod = microsoft::deliveryoptimization::details;

std::string RestApiParser::ParamToString(RestApiParameters param)
{
    return RestApiParam::Lookup(param).stringId;
}

// Parser has lazy-init logic. It takes over the request here and
// uses it later, when requested, to parse the URI, path and JSON body.
RestApiParser::RestApiParser(const std::shared_ptr<microsoft::deliveryoptimization::details::HttpPacket>& request) :
    _request(request)
{
}

RestApiMethods RestApiParser::Method()
{
    using api_map_t = std::map<std::string, std::pair<RestApiMethods, const char*>, case_insensitive_str_less>;
    static const api_map_t supportedAPIs =
        {
            { "create", std::make_pair(RestApiMethods::Create, msdod::http_methods::POST) },
            { "enumerate", std::make_pair(RestApiMethods::Enumerate, msdod::http_methods::GET) },
            { "start", std::make_pair(RestApiMethods::Start, msdod::http_methods::POST) },
            { "pause", std::make_pair(RestApiMethods::Pause, msdod::http_methods::POST) },
            { "finalize", std::make_pair(RestApiMethods::Finalize, msdod::http_methods::POST) },
            { "abort", std::make_pair(RestApiMethods::Abort, msdod::http_methods::POST) },
            { "getstatus", std::make_pair(RestApiMethods::GetStatus, msdod::http_methods::GET) },
            { "getproperty", std::make_pair(RestApiMethods::GetProperty, msdod::http_methods::GET) },
            { "setproperty", std::make_pair(RestApiMethods::SetProperty, msdod::http_methods::POST) },
        };

    if (!_methodInitialized)
    {
        auto paths = msdod::cpprest_web::uri::split_path(msdod::cpprest_web::uri::decode(_request->url.path()));
        if ((paths.size() == 2) && (StringCompareCaseInsensitive(paths[0].data(), "download") == 0))
        {
            auto it = supportedAPIs.find(paths[1]);
            if ((it != supportedAPIs.end()) && (_request->method == it->second.second))
            {
                _method = it->second.first;
                _methodInitialized = true;
            }
        }
    }

    if (!_methodInitialized)
    {
        THROW_HR_MSG(E_UNSUPPORTED, "Unsupported API request: (%s) %s", _request->method.data(), _request->url.to_string().data());
    }
    return _method;
}

const std::string* RestApiParser::QueryStringParam(RestApiParameters param)
{
    auto& queryParams = _QueryParams();
    auto it = queryParams.find(&RestApiParam::Lookup(param));
    if (it != queryParams.end())
    {
        return &(it->second);
    }
    return nullptr;
}

std::string RestApiParser::GetStringParam(RestApiParameters param)
{
    const std::string* str = QueryStringParam(param);
    if (str != nullptr)
    {
        return *str;
    }

    return {};
}

void RestApiParser::_ParseQueryString()
{
    // Search for and store only known parameters.
    // Loop required because json::object does not offer case-insensitive finds.
    // Use std::lower_bound if this loop becomes a bottleneck.
    query_data_t decodedQueryData;
    auto queryData = msdod::cpprest_web::uri::split_query(_request->url.query());
    for (const auto& item : queryData)
    {
        const RestApiParam* param = RestApiParam::Lookup(item.first.data());
        THROW_HR_IF(E_INVALIDARG, param == nullptr);
        // Decode individual query params and not the query string as a whole
        // because it can contain embedded URI with query string that will not
        // get split up correctly if decoded beforehand.
        decodedQueryData[param] = msdod::cpprest_web::uri::decode(item.second);
    }
    _queryData = std::move(decodedQueryData);
}

const RestApiParser::query_data_t& RestApiParser::_QueryParams()
{
    if (!_queryDataInitialized)
    {
        _ParseQueryString();
        _queryDataInitialized = true;
    }
    return _queryData;
}
