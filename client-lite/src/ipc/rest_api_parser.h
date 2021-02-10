#pragma once

#include <cpprest/http_msg.h>
#include <cpprest/json.h>
#include "rest_api_params.h"

enum class RestApiMethods
{
    Create,
    Enumerate,
    Start,
    Pause,
    Finalize,
    Abort,
    GetStatus,
    GetProperty,
    SetProperty,
};

class RestApiParser
{
public:
    using query_data_t = std::map<const RestApiParam*, std::string>;
    using body_data_t = std::map<const RestApiParam*, web::json::value>;

    static std::string ParamToString(RestApiParameters param);

    RestApiParser(web::http::http_request request);

    RestApiMethods Method();
    const std::string* QueryStringParam(RestApiParameters param);
    const web::json::value* BodyParam(RestApiParameters param);
    const body_data_t& Body() { return _Body(); }

    std::string GetStringParam(RestApiParameters param);
    web::uri GetUriParam(RestApiParameters param);

private:
    void _ParseQueryString();
    void _ParseJsonBody(const web::json::value& body);
    const query_data_t& _QueryParams();
    const body_data_t& _Body();

private:
    web::http::http_request _request;

    // All further data members are lazy-init
    RestApiMethods _method;
    query_data_t _queryData;
    body_data_t _bodyData;
    bool _methodInitialized { false };
    bool _queryDataInitialized { false };
    bool _bodyDataInitialized { false };
};
