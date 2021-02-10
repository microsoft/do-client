#include "do_common.h"
#include "rest_api_parser.h"
#include "string_ops.h"

std::string RestApiParser::ParamToString(RestApiParameters param)
{
    return RestApiParam::Lookup(param).stringId;
}

// Parser has lazy-init logic. It takes over the request here and
// uses it later, when requested, to parse the URI, path and JSON body.
RestApiParser::RestApiParser(web::http::http_request request) :
    _request(std::move(request))
{
}

RestApiMethods RestApiParser::Method()
{
    using api_map_t = std::map<std::string, std::pair<RestApiMethods, const web::http::method*>, case_insensitive_str_less>;
    static const api_map_t supportedAPIs =
        {
            { "create", std::make_pair(RestApiMethods::Create, &web::http::methods::POST) },
            { "enumerate", std::make_pair(RestApiMethods::Enumerate, &web::http::methods::GET) },
            { "start", std::make_pair(RestApiMethods::Start, &web::http::methods::POST) },
            { "pause", std::make_pair(RestApiMethods::Pause, &web::http::methods::POST) },
            { "finalize", std::make_pair(RestApiMethods::Finalize, &web::http::methods::POST) },
            { "abort", std::make_pair(RestApiMethods::Abort, &web::http::methods::POST) },
            { "getstatus", std::make_pair(RestApiMethods::GetStatus, &web::http::methods::GET) },
            { "getproperty", std::make_pair(RestApiMethods::GetProperty, &web::http::methods::GET) },
            { "setproperty", std::make_pair(RestApiMethods::SetProperty, &web::http::methods::POST) },
        };

    if (!_methodInitialized)
    {
        auto paths = web::http::uri::split_path(web::http::uri::decode(_request.relative_uri().path()));
        if ((paths.size() == 2) && (StringCompareCaseInsensitive(paths[0].data(), "download") == 0))
        {
            auto it = supportedAPIs.find(paths[1]);
            if ((it != supportedAPIs.end()) && (_request.method() == *(it->second.second)))
            {
                _method = it->second.first;
                _methodInitialized = true;
            }
        }
    }

    if (!_methodInitialized)
    {
        THROW_HR_MSG(E_UNSUPPORTED, "Unsupported API request: (%s) %s", _request.method().data(), _request.relative_uri().to_string().data());
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

const web::json::value* RestApiParser::BodyParam(RestApiParameters param)
{
    auto& bodyData = _Body();
    auto it = bodyData.find(&RestApiParam::Lookup(param));
    if (it != bodyData.end())
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

    const web::json::value* val = BodyParam(param);
    if (val != nullptr)
    {
        return val->as_string();
    }

    return {};
}

web::uri RestApiParser::GetUriParam(RestApiParameters param)
{
    const std::string* str = QueryStringParam(param);
    if (str != nullptr)
    {
        return web::uri(*str);
    }

    const web::json::value* val = BodyParam(param);
    if (val != nullptr)
    {
        return web::uri(val->as_string());
    }

    return {};
}

void RestApiParser::_ParseQueryString()
{
    // Search for and store only known parameters.
    // Loop required because json::object does not offer case-insensitive finds.
    // Use std::lower_bound if this loop becomes a bottleneck.
    query_data_t decodedQueryData;
    auto queryData = web::http::uri::split_query(_request.request_uri().query());
    for (const auto& item : queryData)
    {
        const RestApiParam* param = RestApiParam::Lookup(item.first.data());
        THROW_HR_IF(E_INVALIDARG, param == nullptr);
        // Decode individual query params and not the query string as a whole
        // because it can contain embedded URI with query string that will not
        // get split up correctly if decoded beforehand.
        decodedQueryData[param] = web::http::uri::decode(item.second);
    }
    _queryData = std::move(decodedQueryData);
}

void RestApiParser::_ParseJsonBody(const web::json::value& body)
{
    THROW_HR_IF(E_INVALIDARG, !(body.is_null() || body.is_object()));
    if (body.is_null())
    {
        _bodyData.clear();
        return;
    }

    // Search for and store only known parameters.
    // Loop required because json::object does not offer case-insensitive finds.
    // Use std::lower_bound if this loop becomes a bottleneck.
    body_data_t bodyData;
    for (const auto& val : body.as_object())
    {
        const RestApiParam* param = RestApiParam::Lookup(val.first.data());
        THROW_HR_IF(E_INVALIDARG, param == nullptr);
        bodyData[param] = val.second;
    }
    _bodyData = std::move(bodyData);
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

const RestApiParser::body_data_t& RestApiParser::_Body()
{
    if (!_bodyDataInitialized)
    {
        auto jsonValue = _request.extract_json().get();
        _ParseJsonBody(jsonValue);
        _bodyDataInitialized = true;
    }
    return _bodyData;
}
