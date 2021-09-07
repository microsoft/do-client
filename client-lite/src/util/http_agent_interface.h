#pragma once

enum class HttpAgentHeaders
{
    Range,
};

class IHttpAgent
{
public:
    virtual ~IHttpAgent() = default;
    virtual HRESULT SendRequest(PCSTR url, PCSTR proxyUrl = nullptr, PCSTR range = nullptr, UINT64 callerContext = 0) = 0;
    virtual void Close() = 0;
    virtual HRESULT QueryStatusCode(UINT64 httpContext, _Out_ UINT *statusCode) const = 0;
    virtual HRESULT QueryContentLength(UINT64 httpContext, _Out_ UINT64 *contentLength) = 0;
    virtual HRESULT QueryContentLengthFromRange(UINT64 httpContext, _Out_ UINT64 *contentLength) = 0;
    virtual HRESULT QueryHeaders(UINT64 httpContext, _In_opt_z_ PCSTR name, std::string& headers) const noexcept = 0;
    virtual HRESULT QueryHeadersByType(UINT64 httpContext, HttpAgentHeaders type, std::string& headers) noexcept = 0;
};

class IHttpAgentEvents
{
public:
    virtual ~IHttpAgentEvents() = default;
    virtual HRESULT OnHeadersAvailable(UINT64 httpContext, UINT64 callerContext) = 0;
    virtual HRESULT OnData(_In_reads_bytes_(cbData) BYTE* pData, UINT cbData, UINT64 httpContext, UINT64 callerContext) = 0;
    virtual HRESULT OnComplete(HRESULT hResult, UINT64 httpContext, UINT64 callerContext) = 0;
};
