#pragma once

#include <exception>
#include <stdint.h>
#include <system_error>

namespace microsoft
{
namespace deliveryoptimization
{

enum class errc : int32_t
{
    unexpected              = -2147418113,
    invalid_arg             = -2147024809,
    not_found               = -2147023728,
    no_service              = -2133848063,
    download_no_progress    = -2133843966,
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
}
}