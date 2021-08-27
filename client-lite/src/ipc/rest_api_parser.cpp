#include "do_common.h"
#include "rest_api_parser.h"

#include "do_http_defines.h"
#include "string_ops.h"

namespace msdod = microsoft::deliveryoptimization::details;

// Credits: cpprestsdk
// https://github.com/microsoft/cpprestsdk/blob/master/Release/src/uri/uri.cpp
static std::vector<std::string> g_UriSplitPath(const std::string& path)
{
    std::vector<std::string> results;
    std::stringstream iss(path);
    iss.imbue(std::locale::classic());
    std::string s;
    while (std::getline(iss, s, '/'))
    {
        if (!s.empty())
        {
            results.push_back(s);
        }
    }
    return results;
}
static int g_HexCharDigitToDecimalChar(int hex)
{
    int decimal;
    if (hex >= '0' && hex <= '9')
    {
        decimal = hex - '0';
    }
    else if (hex >= 'A' && hex <= 'F')
    {
        decimal = 10 + (hex - 'A');
    }
    else if (hex >= 'a' && hex <= 'f')
    {
        decimal = 10 + (hex - 'a');
    }
    else
    {
        throw std::invalid_argument("Invalid hexadecimal digit");
    }
    return decimal;
}
static std::string g_UriDecode(const std::string& encoded)
{
    std::string raw;
    for (auto iter = encoded.begin(); iter != encoded.end(); ++iter)
    {
        if (*iter == '%')
        {
            if (++iter == encoded.end())
            {
                throw std::invalid_argument("Invalid URI string, two hexadecimal digits must follow '%'");
            }
            int decimal_value = g_HexCharDigitToDecimalChar(static_cast<int>(*iter)) << 4;
            if (++iter == encoded.end())
            {
                throw std::invalid_argument("Invalid URI string, two hexadecimal digits must follow '%'");
            }
            decimal_value += g_HexCharDigitToDecimalChar(static_cast<int>(*iter));

            raw.push_back(static_cast<char>(decimal_value));
        }
        else if (*iter > 127 || *iter < 0)
        {
            throw std::invalid_argument("Invalid encoded URI string, must be entirely ascii");
        }
        else
        {
            // encoded string has to be ASCII.
            raw.push_back(static_cast<char>(*iter));
        }
    }
    return raw;
}
static std::map<std::string, std::string> g_UriSplitQuery(const std::string& query)
{
    std::map<std::string, std::string> results;

    // Split into key value pairs separated by '&'.
    size_t prev_amp_index = 0;
    while (prev_amp_index != std::string::npos)
    {
        size_t amp_index = query.find_first_of('&', prev_amp_index);
        if (amp_index == std::string::npos) amp_index = query.find_first_of(';', prev_amp_index);

        std::string key_value_pair = query.substr(
            prev_amp_index,
            amp_index == std::string::npos ? query.size() - prev_amp_index : amp_index - prev_amp_index);
        prev_amp_index = amp_index == std::string::npos ? std::string::npos : amp_index + 1;

        size_t equals_index = key_value_pair.find_first_of('=');
        if (equals_index == std::string::npos)
        {
            continue;
        }
        else if (equals_index == 0)
        {
            std::string value(key_value_pair.begin() + equals_index + 1, key_value_pair.end());
            results[std::string {}] = value;
        }
        else
        {
            std::string key(key_value_pair.begin(), key_value_pair.begin() + equals_index);
            std::string value(key_value_pair.begin() + equals_index + 1, key_value_pair.end());
            results[key] = value;
        }
    }

    return results;
}

static std::string g_UriGetQuery(const std::string& url)
{
    auto qmark = url.find('?');
    if (qmark != std::string::npos)
    {
        return url.substr(qmark + 1);
    }
    return std::string{};
}

static std::string g_UriGetPath(const std::string& url)
{
    std::string path;
    size_t firstForwardSlashPos = 0;
    if (url.rfind("http://", 0) == 0)
    {
        // Complete URL, ignore everything until the first '/' after http://
        firstForwardSlashPos = url.find('/', sizeof("http://"));
    }
    else
    {
        firstForwardSlashPos = url.find('/');
    }

    if (firstForwardSlashPos == std::string::npos)
    {
        throw std::invalid_argument("Invalid request URL");
    }

    size_t qmarkPos = url.find('?', firstForwardSlashPos + 1);
    size_t numCharsPath = (qmarkPos == std::string::npos) ? (url.size() - firstForwardSlashPos) : (qmarkPos - firstForwardSlashPos);
    return url.substr(firstForwardSlashPos, numCharsPath);
}

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
        auto paths = g_UriSplitPath(g_UriDecode(g_UriGetPath(_request->url)));
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
        THROW_HR_MSG(E_UNSUPPORTED, "Unsupported API request: (%s) %s", _request->method.data(), _request->url.data());
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
    auto queryData = g_UriSplitQuery(g_UriGetQuery(_request->url));
    for (const auto& item : queryData)
    {
        const RestApiParam* param = RestApiParam::Lookup(item.first.data());
        THROW_HR_IF(E_INVALIDARG, param == nullptr);
        // Decode individual query params and not the query string as a whole
        // because it can contain embedded URI with query string that will not
        // get split up correctly if decoded beforehand.
        decodedQueryData[param] = g_UriDecode(item.second);
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
