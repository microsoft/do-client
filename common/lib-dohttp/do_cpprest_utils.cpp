#include "do_cpprest_utils.h"

namespace microsoft
{
namespace deliveryoptimization
{
namespace details
{
namespace cpprest_utils
{

namespace
{
struct to_lower_ch_impl
{
    char operator()(char c) const CPPREST_NOEXCEPT
    {
        if (c >= 'A' && c <= 'Z') return static_cast<char>(c - 'A' + 'a');
        return c;
    }

    wchar_t operator()(wchar_t c) const CPPREST_NOEXCEPT
    {
        if (c >= L'A' && c <= L'Z') return static_cast<wchar_t>(c - L'A' + L'a');
        return c;
    }
};

CPPREST_CONSTEXPR to_lower_ch_impl to_lower_ch {};
} // namespace

_ASYNCRTIMP void __cdecl inplace_tolower(std::string& target) CPPREST_NOEXCEPT
{
    for (auto& ch : target)
    {
        ch = to_lower_ch(ch);
    }
}

} // namespace cpprest_utils
} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft
