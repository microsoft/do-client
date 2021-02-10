#include "do_common.h"
#include "error_macros.h"

#include <boost/system/system_error.hpp>
#include <cpprest/http_msg.h>   // web::http::http_exception
#include <cpprest/json.h>       // web::json::json_exception
#include <pplx/pplxtasks.h>
#include "string_ops.h"

static docli::logging_callback_type g_pfnLoggingCallback = nullptr;

namespace docli
{
    void SetResultLoggingCallback(logging_callback_type callback)
    {
        g_pfnLoggingCallback = callback;
    }

    void LogFailure(__R_FN_PARAMS_FULL, FailureType type, HRESULT hr, _In_opt_ PCSTR message, _Out_ FailureInfo* failure) DO_NOEXCEPT
    {
        memset(failure, 0, sizeof(*failure));

        failure->type = type;
        failure->hr = hr;
        failure->pszMessage = ((message != nullptr) && (message[0] != '\0')) ? message : nullptr;
        __R_IF_FILE(failure->pszFile = fileName);
        __R_IF_LINE(failure->uLineNumber = lineNumber);
        __R_IF_CODE(failure->pszCode = code);
        __R_IF_FUNCTION(failure->pszFunction = functionName);
#if (RESULT_INCLUDE_CALLER_RETURNADDRESS == 1)
        failure->returnAddress = returnAddress;
        failure->callerReturnAddress = callerReturnAddress;
#endif

        if (g_pfnLoggingCallback)
        {
            g_pfnLoggingCallback(*failure);
        }

        if (SUCCEEDED(failure->hr))
        {
            // TODO(shishirb) support fail-fast
            // Caller bug: Leaking a success code into a failure-only function
            //FAIL_FAST_IMMEDIATE_IF(type != FailureType::FailFast);
            failure->hr = E_UNEXPECTED;
        }
    }

    void ReportFailure(__R_FN_PARAMS_FULL, FailureType type, HRESULT hr, _In_opt_ PCSTR message)
    {
        FailureInfo failure;
        LogFailure(__R_FN_CALL_FULL, type, hr, message, &failure);

        if (type == FailureType::FailFast)
        {
            // TODO(shishirb) support fail-fast
            // WilFailFast(const_cast<FailureInfo&>(failure));
        }
        else if (type == FailureType::Exception)
        {
            throw DOResultException(failure);
        }
    }

    void ReportFailure_Msg(__R_FN_PARAMS_FULL, FailureType type, HRESULT hr, _Printf_format_string_ PCSTR formatString, va_list argList)
    {
        char message[1024];
        PrintLoggingMessage(message, ARRAYSIZE(message), formatString, argList);
        ReportFailure(__R_FN_CALL_FULL, type, hr, message);
    }

    void Return_Hr(__R_FN_PARAMS_FULL, HRESULT hr) DO_NOEXCEPT
    {
        ReportFailure(__R_FN_CALL_FULL, FailureType::Return, hr);
    }

    void Return_HrMsg(__R_FN_PARAMS_FULL, HRESULT hr, _Printf_format_string_ PCSTR formatString, ...) DO_NOEXCEPT
    {
        va_list argList;
        va_start(argList, formatString);
        ReportFailure_Msg(__R_FN_CALL_FULL, FailureType::Return, hr, formatString, argList);
    }

    inline void MaybeGetExceptionString(const DOResultException& exception, _Out_writes_opt_(debugStringChars) PSTR debugString, size_t debugStringChars)
    {
        if (debugString)
        {
            const auto& failureInfo = exception.GetFailureInfo();
#if (RESULT_DIAGNOSTICS_LEVEL >= 5)
            // pszCode is available only in this case
            StringPrintf(debugString, debugStringChars, "DO failure: %s (hr:0x%X) [%s, %d], {%s}",
                failureInfo.pszMessage, failureInfo.hr, failureInfo.pszFile, failureInfo.uLineNumber, failureInfo.pszCode);
#elif (RESULT_DIAGNOSTICS_LEVEL >= 3)
            StringPrintf(debugString, debugStringChars, "DO failure: %s (hr:0x%X) [%s, %d]",
                failureInfo.pszMessage, failureInfo.hr, failureInfo.pszFile, failureInfo.uLineNumber);
#elif (RESULT_DIAGNOSTICS_LEVEL >= 2)
            // pszFile not available in this case
            StringPrintf(debugString, debugStringChars, "DO failure: %s (hr:0x%X) [%d]",
                failureInfo.pszMessage, failureInfo.hr, failureInfo.uLineNumber);
#else
            // uLineNumber also is not available in this case
            StringPrintf(debugString, debugStringChars, "DO failure: %s (hr:0x%X)",
                failureInfo.pszMessage, failureInfo.hr);
#endif
        }
    }

    inline void MaybeGetExceptionString(const std::exception& exception, _Out_writes_opt_(debugStringChars) PSTR debugString, size_t debugStringChars)
    {
        if (debugString)
        {
            StringPrintf(debugString, debugStringChars, "std::exception: %s", exception.what());
        }
    }

    HRESULT ResultFromCaughtExceptionInternal(_Out_writes_opt_(debugStringChars) PSTR debugString, size_t debugStringChars) DO_NOEXCEPT
    {
        if (debugString)
        {
            *debugString = '\0';
        }

        // Note: If we need to handle other exceptions via callbacks, this is where to enable it
        // if (g_pfnResultFromCaughtException)

        try
        {
            throw;
        }
        catch (const DOResultException& exception)
        {
            MaybeGetExceptionString(exception, debugString, debugStringChars);
            return exception.GetErrorCode();
        }
        catch (const pplx::task_canceled&)
        {
            // Either the request failed and we already reported it
            // or the caller did not want to continue with the request.
            return E_ABORT;
        }
        catch (const web::http::http_exception& httpEx)
        {
            if (debugString)
            {
                StringPrintf(debugString, debugStringChars, "http_exception: %s", httpEx.what());
            }
            if (httpEx.error_code() == std::errc::operation_canceled)
            {
                return E_ABORT;
            }
            return HRESULT_FROM_STDCPP(httpEx.error_code());
        }
        catch (const web::json::json_exception& jsonEx)
        {
            if (debugString)
            {
                StringPrintf(debugString, debugStringChars, "json_exception: %s", jsonEx.what());
            }
            return E_INVALIDARG; // json_exception doesn't have an error code
        }
        catch (const boost::system::system_error& sysEx)
        {
            if (debugString)
            {
                StringPrintf(debugString, debugStringChars, "boost_exception: %s", sysEx.what());
            }
            return HRESULT_FROM_BOOST(sysEx.code());
        }
        catch (const std::bad_alloc& exception)
        {
            MaybeGetExceptionString(exception, debugString, debugStringChars);
            return E_OUTOFMEMORY;
        }
        catch (const std::exception& exception)
        {
            MaybeGetExceptionString(exception, debugString, debugStringChars);
            return HRESULT_FROM_WIN32(ERROR_UNHANDLED_EXCEPTION);
        }
        catch (...)
        {
        }

        // Tell the caller that we were unable to map the exception by succeeding...
        return S_OK;
    }

    inline HRESULT ReportFailure_CaughtExceptionCommon(__R_FN_PARAMS_FULL, FailureType type, PSTR debugString, size_t cchDebugString)
    {
        const auto length = strlen(debugString);
        assert(length < cchDebugString);
        HRESULT hr = ResultFromCaughtExceptionInternal(debugString + length, cchDebugString - length);
        const bool known = (FAILED(hr));
        if (!known)
        {
            hr = HRESULT_FROM_WIN32(ERROR_UNHANDLED_EXCEPTION);
            type = FailureType::FailFast;
        }

        ReportFailure(__R_FN_CALL_FULL, type, hr, debugString);
        return hr;
    }

    HRESULT ReportFailure_CaughtException(__R_FN_PARAMS_FULL, FailureType type)
    {
        char message[1024];
        message[0] = '\0';
        return ReportFailure_CaughtExceptionCommon(__R_FN_CALL_FULL, type, message, ARRAYSIZE(message));
    }

    HRESULT ReportFailure_CaughtExceptionMsg(__R_FN_PARAMS_FULL, FailureType type, _Printf_format_string_ PCSTR formatString, va_list argList)
    {
        // Pre-populate the buffer with our message, the exception message will be added to it...
        char message[1024];
        PrintLoggingMessage(message, ARRAYSIZE(message), formatString, argList);
        (void)StringConcatenate(message, ARRAYSIZE(message), " -- ");
        return ReportFailure_CaughtExceptionCommon(__R_FN_CALL_FULL, type, message, ARRAYSIZE(message));
    }

    void Throw_HrMsg(__R_FN_PARAMS_FULL, HRESULT hr, _Printf_format_string_ PCSTR formatString, ...)
    {
        va_list argList;
        va_start(argList, formatString);
        ReportFailure_Msg(__R_FN_CALL_FULL, FailureType::Exception, hr, formatString, argList);
    }

    void Throw_CaughtExceptionMsg(__R_FN_PARAMS_FULL, _Printf_format_string_ PCSTR formatString, ...)
    {
        va_list argList;
        va_start(argList, formatString);
        ReportFailure_CaughtExceptionMsg(__R_FN_CALL_FULL, FailureType::Exception, formatString, argList);
    }

    HRESULT ResultFromCaughtException() DO_NOEXCEPT
    {
        const HRESULT hr = ResultFromCaughtExceptionInternal(nullptr, 0);
        if (FAILED(hr))
        {
            return hr;
        }

        // Caller bug: an unknown exception was thrown
        // TODO(shishirb) fail fast
        return HRESULT_FROM_WIN32(ERROR_UNHANDLED_EXCEPTION);
    }

    HRESULT Log_IfFailed(__R_FN_PARAMS_FULL, HRESULT hr) DO_NOEXCEPT
    {
        if (FAILED(hr))
        {
            ReportFailure(__R_FN_CALL_FULL, FailureType::Log, hr, nullptr);
        }
        return hr;
    }

    HRESULT Log_IfFailedMsg(__R_FN_PARAMS_FULL, HRESULT hr, _Printf_format_string_ PCSTR formatString, ...) DO_NOEXCEPT
    {
        if (FAILED(hr))
        {
            va_list argList;
            va_start(argList, formatString);
            ReportFailure_Msg(__R_FN_CALL_FULL, FailureType::Log, hr, formatString, argList);
        }
        return hr;
    }
} // namespace docli
