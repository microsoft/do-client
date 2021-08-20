#pragma once

#include <cstdarg> // va_start, etc.
#include "basic_types.h"
#include "hresult_helpers.h"
#include "sal_undef.h"
#include "string_ops.h"

//*****************************************************************************
// This is a port of some of the result macros from WIL
// https://github.com/microsoft/wil
//*****************************************************************************

#if (DBG || defined(DEBUG) || defined(_DEBUG)) && !defined(NDEBUG)
#define RESULT_DEBUG
#endif

#ifndef RESULT_DIAGNOSTICS_LEVEL
#if (defined(RESULT_DEBUG) || defined(RESULT_DEBUG_INFO)) && !defined(RESULT_SUPPRESS_DEBUG_INFO)
#define RESULT_DIAGNOSTICS_LEVEL 5
#else
#define RESULT_DIAGNOSTICS_LEVEL 3
#endif
#endif

// DO customization: Return addresses inclusion is disabled until we figure out how to do this in GCC
#define RESULT_INCLUDE_CALLER_RETURNADDRESS 0
#define RESULT_INCLUDE_RETURNADDRESS 0

//*****************************************************************************
// Helpers to setup the macros and functions used below... do not directly use.
//*****************************************************************************
#define __R_COMMA ,

// The following macros assemble the varying amount of data we want to collect from the macros, treating it uniformly
#if (RESULT_DIAGNOSTICS_LEVEL >= 2)  // line number
#define __R_IF_LINE(term) term
#define __R_IF_NOT_LINE(term)
#define __R_IF_COMMA ,
#define __R_LINE_VALUE static_cast<unsigned short>(__LINE__)
#else
#define __R_IF_LINE(term)
#define __R_IF_NOT_LINE(term) term
#define __R_IF_COMMA
#define __R_LINE_VALUE static_cast<unsigned short>(0)
#endif
#if (RESULT_DIAGNOSTICS_LEVEL >= 3) // line number + file name
#define __R_IF_FILE(term) term
#define __R_IF_NOT_FILE(term)
#define __R_FILE_VALUE __FILE__
#else
#define __R_IF_FILE(term)
#define __R_IF_NOT_FILE(term) term
#define __R_FILE_VALUE nullptr
#endif
#if (RESULT_DIAGNOSTICS_LEVEL >= 4) // line number + file name + function name
#define __R_IF_FUNCTION(term) term
#define __R_IF_NOT_FUNCTION(term)
#else
#define __R_IF_FUNCTION(term)
#define __R_IF_NOT_FUNCTION(term) term
#endif
#if (RESULT_DIAGNOSTICS_LEVEL >= 5) // line number + file name + function name + macro code
#define __R_IF_CODE(term) term
#define __R_IF_NOT_CODE(term)
#else
#define __R_IF_CODE(term)
#define __R_IF_NOT_CODE(term) term
#endif
#if (RESULT_INCLUDE_CALLER_RETURNADDRESS == 1)
#define __R_IF_CALLERADDRESS(term) term
#define __R_IF_NOT_CALLERADDRESS(term)
#define __R_CALLERADDRESS_VALUE _ReturnAddress()
#else
#define __R_IF_CALLERADDRESS(term)
#define __R_IF_NOT_CALLERADDRESS(term) term
#define __R_CALLERADDRESS_VALUE nullptr
#endif

#if (RESULT_INCLUDE_CALLER_RETURNADDRESS == 1) || (RESULT_DIAGNOSTICS_LEVEL >= 2)
#define __R_IF_TRAIL_COMMA ,
#else
#define __R_IF_TRAIL_COMMA
#endif

#define __R_ENABLE_IF_IS_CLASS(ptrType)                     typename std::enable_if_t<std::is_class<ptrType>::value, void*> = (void*)0
#define __R_ENABLE_IF_IS_NOT_CLASS(ptrType)                 typename std::enable_if_t<!std::is_class<ptrType>::value, void*> = (void*)0

// Assemble the varying amounts of data into a single macro
#define __R_INFO_ONLY(CODE)                                 __R_IF_CALLERADDRESS(_ReturnAddress() __R_IF_COMMA) __R_IF_LINE(__R_LINE_VALUE) __R_IF_FILE(__R_COMMA __R_FILE_VALUE) __R_IF_FUNCTION(__R_COMMA __FUNCTION__) __R_IF_CODE(__R_COMMA CODE)
#define __R_INFO(CODE)                                      __R_INFO_ONLY(CODE) __R_IF_TRAIL_COMMA

#define __R_FN_PARAMS_FULL                                  __R_IF_LINE(unsigned int lineNumber __R_IF_COMMA) __R_IF_FILE(_In_opt_ PCSTR fileName) __R_IF_FUNCTION(__R_COMMA _In_opt_ PCSTR functionName) __R_IF_CODE(__R_COMMA _In_opt_ PCSTR code)
#define __R_FN_CALL_FULL                                    __R_IF_LINE(lineNumber __R_IF_COMMA) __R_IF_FILE(fileName) __R_IF_FUNCTION(__R_COMMA functionName) __R_IF_CODE(__R_COMMA code)

// Helpers for return macros
#define __RETURN_HR_MSG(hr, str, fmt, ...)                  do { HRESULT __hr = (hr); if (FAILED(__hr)) { docli::Return_HrMsg(__R_INFO(str) __hr, fmt, ##__VA_ARGS__); } return __hr; } while (0)
#define __RETURN_HR_MSG_FAIL(hr, str, fmt, ...)             do { HRESULT __hr = (hr); docli::Return_HrMsg(__R_INFO(str) __hr, fmt, ##__VA_ARGS__); return __hr; } while (0)
#define __RETURN_HR(hr, str)                                do { HRESULT __hr = (hr); if (FAILED(__hr)) { docli::Return_Hr(__R_INFO(str) __hr); } return __hr; } while (0)
#define __RETURN_HR_FAIL(hr, str)                           do { HRESULT __hr = (hr); docli::Return_Hr(__R_INFO(str) __hr); return __hr; } while (0)

//*****************************************************************************
// Macros for returning failures as HRESULTs
//*****************************************************************************

// '##' is required before __VA_ARGS__ to allow an empty arg list to the variadic macro.
// MSVC supports this by default but GCC requires '##'. C++2a has added VA_OPT macro
// to officially support this behavior.

// Always returns a known result (HRESULT) - always logs failures
#define RETURN_HR(hr)                                           __RETURN_HR(docli::verify_hresult(hr), #hr)

// Always returns a known failure (HRESULT) - always logs a var-arg message on failure
#define RETURN_HR_MSG(hr, fmt, ...)                             __RETURN_HR_MSG(docli::verify_hresult(hr), #hr, fmt, ##__VA_ARGS__)

// Conditionally returns failures (HRESULT) - always logs failures
#define RETURN_IF_FAILED(hr)                                    do { HRESULT __hrRet = docli::verify_hresult(hr); if (FAILED(__hrRet)) { __RETURN_HR_FAIL(__hrRet, #hr); }} while (0)
#define RETURN_HR_IF(hr, condition)                             do { if (docli::verify_bool(condition)) { __RETURN_HR(docli::verify_hresult(hr), #condition); }} while (0)
#define RETURN_HR_IF_NULL(hr, ptr)                              do { if ((ptr) == nullptr) { __RETURN_HR(docli::verify_hresult(hr), #ptr); }} while (0)

// Conditionally returns failures (HRESULT) - always logs a var-arg message on failure
#define RETURN_IF_FAILED_MSG(hr, fmt, ...)                      do { auto __hrRet = docli::verify_hresult(hr); if (FAILED(__hrRet)) { __RETURN_HR_MSG_FAIL(__hrRet, #hr, fmt, ##__VA_ARGS__); }} while (0)
#define RETURN_HR_IF_MSG(hr, condition, fmt, ...)               do { if (docli::verify_bool(condition)) { __RETURN_HR_MSG(docli::verify_hresult(hr), #condition, fmt, ##__VA_ARGS__); }} while (0)

// Conditionally returns failures (HRESULT) - use for failures that are expected in common use - failures are not logged - macros are only for control flow pattern
#define RETURN_IF_FAILED_EXPECTED(hr)                           do { auto __hrRet = docli::verify_hresult(hr); if (FAILED(__hrRet)) { return __hrRet; }} while (0)
#define RETURN_HR_IF_EXPECTED(hr, condition)                    do { if (docli::verify_bool(condition)) { return docli::verify_hresult(hr); }} while (0)

// DEPRECATED: Use RETURN_HR_IF(hr, !condition)
#define RETURN_HR_IF_FALSE(hr, condition)                       RETURN_HR_IF(hr, !(docli::verify_bool(condition)))
// DEPRECATED: Use RETURN_HR_IF_EXPECTED(hr, !condition)
#define RETURN_HR_IF_FALSE_EXPECTED(hr, condition)              RETURN_HR_IF_EXPECTED(hr, !(docli::verify_bool(condition)))

//*****************************************************************************
// Macros to throw exceptions on failure
//*****************************************************************************

// Always throw a known failure
#define THROW_HR(hr)                                            docli::Throw_Hr(__R_INFO(#hr) docli::verify_hresult(hr))
#define THROW_HR_IF(hr, condition)                              docli::Throw_HrIf(__R_INFO(#condition) docli::verify_hresult(hr), docli::verify_bool(condition))
#define THROW_HR_IF_NULL(hr, ptr)                               docli::Throw_HrIfNull(__R_INFO(#ptr) docli::verify_hresult(hr), ptr)

// Conditionally throw failures - returns parameter value
#define THROW_IF_FAILED(hr)                                     docli::Throw_IfFailed(__R_INFO(#hr) docli::verify_hresult(hr))

// Always throw a known failure - throw a var-arg message on failure
#define THROW_HR_MSG(hr, fmt, ...)                              docli::Throw_HrMsg(__R_INFO(#hr) docli::verify_hresult(hr), fmt, ##__VA_ARGS__)

//*****************************************************************************
// Macros to catch and convert exceptions on failure
//*****************************************************************************

// Use these macros *within* a catch (...) block to handle exceptions
#define RETURN_CAUGHT_EXCEPTION()                               return docli::Return_CaughtException(__R_INFO_ONLY(nullptr))
#define RETURN_CAUGHT_EXCEPTION_MSG(fmt, ...)                   return docli::Return_CaughtExceptionMsg(__R_INFO(nullptr) fmt, ##__VA_ARGS__)
#define LOG_CAUGHT_EXCEPTION()                                  docli::Log_CaughtException(__R_INFO_ONLY(nullptr))
#define LOG_CAUGHT_EXCEPTION_MSG(fmt, ...)                      docli::Log_CaughtExceptionMsg(__R_INFO(nullptr) fmt, ##__VA_ARGS__)
#define THROW_NORMALIZED_CAUGHT_EXCEPTION_MSG(fmt, ...)         docli::Throw_CaughtExceptionMsg(__R_INFO(nullptr) fmt, ##__VA_ARGS__)

// Use these macros in place of a catch block to handle exceptions
#define CATCH_RETURN()                                          catch (...) { RETURN_CAUGHT_EXCEPTION(); }
#define CATCH_RETURN_MSG(fmt, ...)                              catch (...) { RETURN_CAUGHT_EXCEPTION_MSG(fmt, ##__VA_ARGS__); }
#define CATCH_LOG()                                             catch (...) { LOG_CAUGHT_EXCEPTION(); }
#define CATCH_THROW_NORMALIZED_MSG(fmt, ...)                    catch (...) { THROW_NORMALIZED_CAUGHT_EXCEPTION_MSG(fmt, ##__VA_ARGS__); }

//*****************************************************************************
// Macros for logging failures (ignore or pass-through)
//*****************************************************************************

// Always logs a known failure - logs a var-arg message on failure
#define LOG_HR_MSG(hr, fmt, ...)                                docli::Log_HrMsg(__R_INFO(#hr) docli::verify_hresult(hr), fmt, ##__VA_ARGS__)

// Conditionally logs failures - returns parameter value
#define LOG_HR_IF(hr, condition)                                docli::Log_HrIf(__R_INFO(#condition) docli::verify_hresult(hr), docli::verify_bool(condition))

// Conditionally logs failures - returns parameter value
#define LOG_IF_FAILED(hr)                                       docli::Log_IfFailed(__R_INFO(#hr) docli::verify_hresult(hr))

// Conditionally logs failures - returns parameter value - logs a var-arg message on failure
#define LOG_IF_FAILED_MSG(hr, fmt, ...)                         docli::Log_IfFailedMsg(__R_INFO(#hr) docli::verify_hresult(hr), fmt, ##__VA_ARGS__)

namespace docli
{
#define DO_NOEXCEPT noexcept

    // Indicates the kind of message / failure type that was used to produce a given error
    enum class FailureType
    {
        Exception,          // THROW_...
        Return,             // RETURN_..._LOG or RETURN_..._MSG
        Log,                // LOG_...
        FailFast,           // FAIL_FAST_...
    };

    // Represents all context information about a given failure
    // No constructors, destructors or virtual members should be contained within
    struct FailureInfo
    {
        FailureType type;
        HRESULT hr;
        PCSTR pszMessage;                       // Message is only present for _MSG logging (it's the sprintf message)
        PCSTR pszCode;                          // [debug only] Capture code from the macro
        PCSTR pszFunction;                      // [debug only] The function name
        PCSTR pszFile;
        unsigned int uLineNumber;
    };

    // A RAII wrapper around the storage of a FailureInfo struct (which is normally meant to be consumed
    // on the stack or from the caller). The storage of FailureInfo needs to copy the pszMessage string
    // to maintain its lifetime. The other string members in FailureInfo are always read-only strings
    // from the binary image so there is no need to copy them (DO customization).
    class StoredFailureInfo
    {
    public:
        StoredFailureInfo() DO_NOEXCEPT
        {
            memset(&_failureInfo, 0, sizeof(_failureInfo));
        }

        StoredFailureInfo(const FailureInfo& failureInfo) DO_NOEXCEPT
        {
            _failureInfo = failureInfo;
            try
            {
                if (failureInfo.pszMessage)
                {
                    _msg = failureInfo.pszMessage;
                    _failureInfo.pszMessage = _msg.data();
                }
            }
            catch (std::bad_alloc&)
            {
                // ignore, can't do anything here
            }
        }

        const FailureInfo& Get() const DO_NOEXCEPT
        {
            return _failureInfo;
        }

    private:
        FailureInfo _failureInfo;
        std::string _msg;
    };

    // This is the exception class thrown from all THROW_XXX macros.
    // This class stores all of the FailureInfo context that is available when the exception is thrown.
    // It's also caught by exception guards for automatic conversion to HRESULT.
    // Note: DO customization: what() is not overriden.
    class DOResultException : public std::exception
    {
    public:
        DOResultException(const FailureInfo& failure) DO_NOEXCEPT :
            _failureInfo(failure)
        {
        }

        // Returns the failed HRESULT that this exception represents.
        HRESULT GetErrorCode() const DO_NOEXCEPT
        {
            return _failureInfo.Get().hr;
        }

        // Get a reference to the stored FailureInfo.
        const FailureInfo& GetFailureInfo() const DO_NOEXCEPT
        {
            return _failureInfo.Get();
        }

        // Sets the stored FailureInfo (use primarily only when constructing custom exception types).
        void SetFailureInfo(const FailureInfo& failure) DO_NOEXCEPT
        {
            _failureInfo = failure;
        }

        // Relies upon auto-generated copy constructor and assignment operator

    protected:
        StoredFailureInfo _failureInfo;                // The failure information for this exception
    };

    // Observe all errors flowing through the system with this callback (set with docli::SetResultLoggingCallback); use with custom logging
    using logging_callback_type = void(*)(const docli::FailureInfo& failure) DO_NOEXCEPT;

    void SetResultLoggingCallback(logging_callback_type callback);

    template <typename T>
    HRESULT verify_hresult(T hr)
    {
        static_assert(sizeof(T) == 4, "Wrong Size: HRESULT expected to be 4 bytes");
#ifdef _WIN32
        static_assert(std::is_same<T, long>::value, "Wrong Type: HRESULT expected");
#else
        static_assert(std::is_same<T, int32_t>::value, "Wrong Type: HRESULT expected");
#endif
        return hr;
    }

    // Type Validation
    // Helpers to validate variable types to prevent accidental, but allowed type conversions.
    // These helpers are most useful when building macros that accept a particular type.  Putting these functions around the types accepted
    // prior to pushing that type through to a function (or using it within the macro) allows the macro to add an additional layer of type
    // safety that would ordinarily be stripped away by C++ implicit conversions.  This system is extensively used in the error handling helper
    // macros to validate the types given to various macro parameters.

    // Verify that val can be evaluated as a logical bool.
    // Other types will generate an intentional compilation error. Allowed types for a logical bool are bool, BOOL,
    // boolean, BOOLEAN, and classes with an explicit bool cast.
    // Returns a C++ bool representing the evaluation of val.
    template <typename T, __R_ENABLE_IF_IS_CLASS(T)>
    bool verify_bool(const T& val)
    {
        return static_cast<bool>(val);
    }

    template <typename T, __R_ENABLE_IF_IS_NOT_CLASS(T)>
    bool verify_bool(T val)
    {
        static_assert(!std::is_same<T, T>::value, "Wrong Type: bool/BOOL/BOOLEAN/boolean expected");
        return static_cast<bool>(val);
    }

    template <>
    inline bool verify_bool<bool>(bool val)
    {
        return val;
    }

    template <>
    inline bool verify_bool<int>(int val)
    {
        return (val != 0);
    }

    template <>
    inline bool verify_bool<unsigned char>(unsigned char val)
    {
        return !!val;
    }

    // TODO why does WIL mark some functions as inline + __declspec(noinline) and some as just inline?

    void LogFailure(__R_FN_PARAMS_FULL, FailureType type, HRESULT hr, _In_opt_ PCSTR message, _Out_ FailureInfo* failure) DO_NOEXCEPT;

    void ReportFailure(__R_FN_PARAMS_FULL, FailureType type, HRESULT hr, _In_opt_ PCSTR message = nullptr);

    void ReportFailure_Msg(__R_FN_PARAMS_FULL, FailureType type, HRESULT hr, _Printf_format_string_ PCSTR formatString, va_list argList);

    inline void PrintLoggingMessage(PSTR pszDest, size_t cchDest, _In_opt_ _Printf_format_string_ PCSTR formatString, _In_opt_ va_list argList) DO_NOEXCEPT
    {
        if (formatString == nullptr)
        {
            pszDest[0] = '\0';
        }
        else
        {
            int cchWritten;
            StringPrintfV(pszDest, cchDest, &cchWritten, formatString, argList);
        }
    }

    void Return_Hr(__R_FN_PARAMS_FULL, HRESULT hr) DO_NOEXCEPT;
    void Return_HrMsg(__R_FN_PARAMS_FULL, HRESULT hr, _Printf_format_string_ PCSTR formatString, ...) DO_NOEXCEPT;

    inline void Throw_Hr(__R_FN_PARAMS_FULL, HRESULT hr)
    {
        docli::ReportFailure(__R_FN_CALL_FULL, FailureType::Exception, hr);
    }

    inline bool Throw_HrIf(__R_FN_PARAMS_FULL, HRESULT hr, bool condition)
    {
        if (condition)
        {
            Throw_Hr(__R_FN_CALL_FULL, hr);
        }
        return condition;
    }

    template <typename PointerT, __R_ENABLE_IF_IS_NOT_CLASS(PointerT)>
    PointerT Throw_HrIfNull(__R_FN_PARAMS_FULL, HRESULT hr, PointerT pointer)
    {
        if (pointer == nullptr)
        {
            Throw_Hr(__R_FN_CALL_FULL, hr);
        }
        return pointer;
    }

    template <typename PointerT, __R_ENABLE_IF_IS_CLASS(PointerT)>
    void Throw_HrIfNull(__R_FN_PARAMS_FULL, HRESULT hr, const PointerT& pointer)
    {
        if (pointer == nullptr)
        {
            Throw_Hr(__R_FN_CALL_FULL, hr);
        }
    }

    inline HRESULT Throw_IfFailed(__R_FN_PARAMS_FULL, HRESULT hr)
    {
        if (FAILED(hr))
        {
            Throw_Hr(__R_FN_CALL_FULL, hr);
        }
        return hr;
    }

    HRESULT ReportFailure_CaughtException(__R_FN_PARAMS_FULL, FailureType type);
    HRESULT ReportFailure_CaughtExceptionMsg(__R_FN_PARAMS_FULL, FailureType type, _Printf_format_string_ PCSTR formatString, va_list argList);

    inline HRESULT Return_CaughtException(__R_FN_PARAMS_FULL) DO_NOEXCEPT
    {
        return docli::ReportFailure_CaughtException(__R_FN_CALL_FULL, FailureType::Return);
    }

    inline HRESULT Return_CaughtExceptionMsg(__R_FN_PARAMS_FULL, _Printf_format_string_ PCSTR formatString, ...) DO_NOEXCEPT
    {
        va_list argList;
        va_start(argList, formatString);
        return docli::ReportFailure_CaughtExceptionMsg(__R_FN_CALL_FULL, FailureType::Return, formatString, argList);
    }

    inline HRESULT Log_CaughtException(__R_FN_PARAMS_FULL) DO_NOEXCEPT
    {
        return docli::ReportFailure_CaughtException(__R_FN_CALL_FULL, FailureType::Log);
    }

    inline HRESULT Log_CaughtExceptionMsg(__R_FN_PARAMS_FULL, _Printf_format_string_ PCSTR formatString, ...) DO_NOEXCEPT
    {
        va_list argList;
        va_start(argList, formatString);
        return docli::ReportFailure_CaughtExceptionMsg(__R_FN_CALL_FULL, FailureType::Log, formatString, argList);
    }

    inline HRESULT Log_HrMsg(__R_FN_PARAMS_FULL, HRESULT hr, _Printf_format_string_ PCSTR formatString, ...) DO_NOEXCEPT
    {
        va_list argList;
        va_start(argList, formatString);
        ReportFailure_Msg(__R_FN_CALL_FULL, FailureType::Log, hr, formatString, argList);
        return hr;
    }

    inline bool Log_HrIf(__R_FN_PARAMS_FULL, HRESULT hr, bool condition) DO_NOEXCEPT
    {
        if (condition)
        {
            ReportFailure(__R_FN_CALL_FULL, FailureType::Log, hr);
        }
        return condition;
    }

    void Throw_HrMsg(__R_FN_PARAMS_FULL, HRESULT hr, _Printf_format_string_ PCSTR formatString, ...);
    void Throw_CaughtExceptionMsg(__R_FN_PARAMS_FULL, _Printf_format_string_ PCSTR formatString, ...);

    // ResultFromCaughtException is a function that is meant to be called from within a catch(...) block.  Internally
    // it re-throws and catches the exception to convert it to an HRESULT. If an exception is of an unrecognized type
    // the function will fail fast.
    HRESULT ResultFromCaughtException() DO_NOEXCEPT;

    HRESULT Log_IfFailed(__R_FN_PARAMS_FULL, HRESULT hr) DO_NOEXCEPT;
    HRESULT Log_IfFailedMsg(__R_FN_PARAMS_FULL, HRESULT hr, _Printf_format_string_ PCSTR formatString, ...) DO_NOEXCEPT;
}
