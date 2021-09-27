// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <boost/system/error_code.hpp>
#include "basic_types.h"

#ifndef _WIN32

#define FACILITY_WIN32                  7
#define FACILITY_DELIVERY_OPTIMIZATION  208
#define HRESULT_FACILITY(hr)            (((hr) >> 16) & 0x1FFF)
#define HRESULT_CODE(hr)                ((hr) & 0xFFFF)

#define ERROR_FILE_NOT_FOUND             2L
#define ERROR_NOT_ENOUGH_MEMORY          8L
#define ERROR_INVALID_DATA               13L
#define ERROR_BAD_LENGTH                 24L
#define ERROR_SHARING_VIOLATION          32L
#define ERROR_FILE_EXISTS                80L
#define ERROR_DISK_FULL                  112L
#define ERROR_ALREADY_EXISTS             183L
#define ERROR_FILE_TOO_LARGE             223L
#define ERROR_NO_MORE_ITEMS              259L
#define ERROR_UNHANDLED_EXCEPTION        574L
#define ERROR_OPERATION_ABORTED          955L
#define ERROR_IO_PENDING                 997L
#define ERROR_NOT_FOUND                  1168L
#define ERROR_DISK_QUOTA_EXCEEDED        1295L
#define ERROR_NO_SYSTEM_RESOURCES        1450L
#define ERROR_TIMEOUT                    1460L
#define ERROR_INVALID_DATATYPE           1804L
#define ERROR_INVALID_STATE              5023L

#define HTTP_STATUS_OK                  200 // request completed
#define HTTP_STATUS_FORBIDDEN           403 // request forbidden
#define HTTP_STATUS_PARTIAL_CONTENT     206 // partial GET fulfilled

inline constexpr HRESULT HRESULT_FROM_WIN32(unsigned long x)
{
    return (HRESULT)(x) <= 0 ? (HRESULT)(x) : (HRESULT) (((x) & 0x0000FFFF) | (FACILITY_WIN32 << 16) | 0x80000000);
}

#define S_OK            ((HRESULT)0L)
#define S_FALSE         ((HRESULT)1L)
#define E_NOTIMPL       ((HRESULT)0x80004001L)
#define E_ABORT         ((HRESULT)0x80004004L)
#define E_FAIL          ((HRESULT)0x80004005L)
#define E_ACCESSDENIED  ((HRESULT)0x80070005L)
#define E_UNEXPECTED    ((HRESULT)0x8000FFFFL)

#define E_OUTOFMEMORY       ((HRESULT)0x8007000EL)
#define E_INVALIDARG        ((HRESULT)0x80070057L)
#define E_NOT_SET           ((HRESULT)0x80070490L)
#define E_NOT_VALID_STATE   HRESULT_FROM_WIN32(ERROR_INVALID_STATE)

#define WININET_E_TIMEOUT                   ((HRESULT)0x80072EE2L)
#define WININET_E_HEADER_NOT_FOUND          ((HRESULT)0x80072F76L)
#define INET_E_INVALID_URL                  ((HRESULT)0x800C0002L)

#define HTTP_E_STATUS_UNEXPECTED                ((HRESULT)0x80190001L)
#define HTTP_E_STATUS_UNEXPECTED_REDIRECTION    ((HRESULT)0x80190003L)
#define HTTP_E_STATUS_BAD_REQUEST               ((HRESULT)0x80190190L)
#define HTTP_E_STATUS_DENIED                    ((HRESULT)0x80190191L)
#define HTTP_E_STATUS_FORBIDDEN                 ((HRESULT)0x80190193L)
#define HTTP_E_STATUS_NOT_FOUND                 ((HRESULT)0x80190194L)
#define HTTP_E_STATUS_NONE_ACCEPTABLE           ((HRESULT)0x80190196L)
#define HTTP_E_STATUS_PROXY_AUTH_REQ            ((HRESULT)0x80190197L)
#define HTTP_E_STATUS_REQUEST_TIMEOUT           ((HRESULT)0x80190198L)
#define HTTP_E_STATUS_REQUEST_TOO_LARGE         ((HRESULT)0x8019019DL)
#define HTTP_E_STATUS_SERVER_ERROR              ((HRESULT)0x801901F4L)
#define HTTP_E_STATUS_NOT_SUPPORTED             ((HRESULT)0x801901F5L)
#define HTTP_E_STATUS_BAD_GATEWAY               ((HRESULT)0x801901F6L)

#define WEB_E_JSON_VALUE_NOT_FOUND          ((HRESULT)0x83750009L)

#define SUCCEEDED(hr)   (((HRESULT)(hr)) >= 0)
#define FAILED(hr)      (((HRESULT)(hr)) < 0)

static_assert(SUCCEEDED(S_OK), "SUCCEEDED macro does not recognize S_OK");
static_assert(SUCCEEDED(S_FALSE), "SUCCEEDED macro does not recognize S_FALSE");
static_assert(FAILED(E_NOT_SET), "FAILED macro does not recognize failure code");

#endif // !_WIN32

#define E_UNSUPPORTED       ((HRESULT)0x80070032L)  // 0x32 = 50L = ERROR_NOT_SUPPORTED

#ifndef STRSAFE_E_INSUFFICIENT_BUFFER
#define STRSAFE_E_INSUFFICIENT_BUFFER       ((HRESULT)0x8007007AL)  // 0x7A = 122L = ERROR_INSUFFICIENT_BUFFER
#endif

#ifndef WINHTTP_ERROR_BASE
#define WINHTTP_ERROR_BASE                     12000
#define ERROR_WINHTTP_TIMEOUT                  (WINHTTP_ERROR_BASE + 2)
#define ERROR_WINHTTP_UNRECOGNIZED_SCHEME      (WINHTTP_ERROR_BASE + 6)
#define ERROR_WINHTTP_NAME_NOT_RESOLVED        (WINHTTP_ERROR_BASE + 7)
#define ERROR_WINHTTP_CANNOT_CONNECT           (WINHTTP_ERROR_BASE + 29)
#endif

// Convert std c++ and boost errors to NTSTATUS-like values but with 0xD0 facility (0xC0D00005 for example).
#define HRESULT_FROM_XPLAT_SYSERR(err)  (HRESULT)(0xC0000000 | (FACILITY_DELIVERY_OPTIMIZATION << 16) | ((HRESULT)(err) & 0x0000FFFF))

inline HRESULT HRESULT_FROM_STDCPP(const std::error_code& ec)
{
    return ec ? HRESULT_FROM_XPLAT_SYSERR(ec.value()) : S_OK;
}

inline HRESULT HRESULT_FROM_BOOST(const boost::system::error_code& ec)
{
    return ec ? HRESULT_FROM_XPLAT_SYSERR(ec.value()) : S_OK;
}
