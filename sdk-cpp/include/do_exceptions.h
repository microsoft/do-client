#ifndef _DELIVERY_OPTIMIZATION_DO_EXCEPTIONS_H
#define _DELIVERY_OPTIMIZATION_DO_EXCEPTIONS_H

#if (!DO_DISABLE_EXCEPTIONS)

#include <exception>
#include <stdint.h>
#include <system_error>

#endif // !DO_DISABLE_EXCEPTIONS

namespace microsoft
{
namespace deliveryoptimization
{

// Error Macros
#ifndef RETURN_HR_IF_FAILED
#define RETURN_HR_IF_FAILED(hr)  {  \
    int32_t __hr = (hr);            \
    if(FAILED(__hr)) return __hr;   \
}
#endif

#ifndef FACILITY_DELIVERY_OPTIMIZATION 
#define FACILITY_DELIVERY_OPTIMIZATION   208
#endif

#define HRESULT_FROM_SYSTEM_ERROR(x) ((int32_t)(x) <= 0 ? ((int32_t)(x)) : ((int32_t) (((x) & 0x0000FFFF) | (FACILITY_DELIVERY_OPTIMIZATION << 16) | 0x80000000)))

#ifndef FAILED
#define FAILED(hr) (((int32_t)(hr)) < 0)
#endif

#ifndef SUCCEEDED
#define SUCCEEDED(hr) (((int32_t)(hr)) >= 0)
#endif

#if (!DO_DISABLE_EXCEPTIONS)

enum class errc : int32_t
{
    ok                          = 0,
    unexpected                  = -2147418113,
    invalid_arg                 = -2147024809,
    not_found                   = -2147023728,
    no_service                  = -2133848063,
    download_no_progress        = -2133843966,
    do_e_invalid_state          = -2133843949, // TODO: Revisit convention here - should separate error code enum be used for do_e* errors?
    do_e_unknown_property_id    = -2133843951
};

class error_category : public std::error_category
{
public:
    const char* name() const noexcept override;

    std::string message(int code) const override;
};

class exception : public std::exception
{
public:
    exception(std::error_code code);
    exception(int32_t code);
    exception(errc code);

    const char* what() const noexcept override;

    int32_t error_code() const;

    const std::error_code& get_error_code() const;

private:
    std::error_code _code;
    std::string _msg;
};

inline void throw_if_fail(int32_t hr)
{
    if (FAILED(hr))
    {
        throw exception(hr);
    }
}
#endif // !DO_DISABLE_EXCEPTIONS
}
}

#endif
