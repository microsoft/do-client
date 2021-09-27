// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

enum class HttpAgentHeaders
{
    Range,
};

class IHttpAgent
{
public:
    virtual ~IHttpAgent() = default;
    virtual HRESULT SendRequest(PCSTR url, PCSTR proxyUrl = nullptr, PCSTR range = nullptr) = 0;
    virtual void Close() = 0;
    virtual HRESULT QueryStatusCode(_Out_ UINT *statusCode) const = 0;
    virtual HRESULT QueryContentLength(_Out_ UINT64 *contentLength) = 0;
    virtual HRESULT QueryContentLengthFromRange(_Out_ UINT64 *contentLength) = 0;
    virtual HRESULT QueryHeaders(_In_opt_z_ PCSTR name, std::string& headers) const noexcept = 0;
    virtual HRESULT QueryHeadersByType(HttpAgentHeaders type, std::string& headers) noexcept = 0;
};

class IHttpAgentEvents
{
public:
    virtual ~IHttpAgentEvents() = default;
    virtual HRESULT OnHeadersAvailable() = 0;
    virtual HRESULT OnData(_In_reads_bytes_(cbData) BYTE* pData, UINT cbData) = 0;
    virtual HRESULT OnComplete(HRESULT hResult) = 0;
};
