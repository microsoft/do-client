#ifndef _DELIVERY_OPTIMIZATION_DO_EXCEPTIONS_H
#define _DELIVERY_OPTIMIZATION_DO_EXCEPTIONS_H

#include <stdint.h>
#include <system_error>

#if (!DO_DISABLE_EXCEPTIONS)
#include <exception>
#endif

namespace microsoft
{
namespace deliveryoptimization
{

//These error macros are defined behind guards so that they do not redefine the macro for for platforms/toolchains in which they already exist
#ifndef RETURN_RES_IF_FAILED
#define RETURN_RES_IF_FAILED(res)  {  \
    int32_t __res = (res);            \
    if(FAILED(__res)) return __res;   \
}
#endif

#ifndef FACILITY_DELIVERY_OPTIMIZATION 
#define FACILITY_DELIVERY_OPTIMIZATION   208
#endif

#define DO_ERROR_FROM_SYSTEM_ERROR(x) (int32_t)(0xC0000000 | (0xD0 << 16) | ((int32_t)(x) & 0x0000FFFF))
#define DO_ERROR_FROM_STD_ERROR(x) ((int32_t)(x) <= 0 ? ((int32_t)(x)) : ((int32_t) (((int32_t)(x) & 0x0000FFFF) | (FACILITY_DELIVERY_OPTIMIZATION << 16) | 0x80000000)))

#ifndef FAILED
#define FAILED(res) (((int32_t)(res)) < 0)
#endif

#ifndef SUCCEEDED
#define SUCCEEDED(res) (((int32_t)(res)) >= 0)
#endif

enum class errc : int32_t
{
    s_ok                        = 0,
    e_not_impl                  = -2063400958,
    unexpected                  = -2147418113,
    invalid_arg                 = -2147024809,
    not_found                   = -2147023728,
    no_service                  = -2133848063,
    download_no_progress        = -2133843966,
    do_e_invalid_state          = -2133843949, // TODO: Revisit convention here - should separate error code enum be used for do_e* errors?
    do_e_unknown_property_id    = -2133843951
};

#if (!DO_DISABLE_EXCEPTIONS)

class error_category : public std::error_category
{
public:
    const char* name() const noexcept override;

    std::string message(int code) const override;
};

class exception : public std::exception
{
public:
    // TODO(jimson): With the error macro above, std::error_code is always transformed into an int32_t before throwing
    // Another option could have been creating an error code class which accepts std::error_code, int32_t, and errc as constructor args
    // Look into deprecating this interface when we publish a new MajorVersion
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

inline void throw_if_fail(int32_t res)
{
    if (FAILED(res))
    {
        throw exception(res);
    }
}

#endif // !DO_DISABLE_EXCEPTIONS

} //namespace deliveryoptimization
} //namespace microsoft

#endif
