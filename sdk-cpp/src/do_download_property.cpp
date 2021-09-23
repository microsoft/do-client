

#include "do_download_property.h"

#include "do_download_property_internal.h"

#include "do_exceptions.h"

namespace msdod = microsoft::deliveryoptimization::details;

namespace microsoft
{
namespace deliveryoptimization
{

download_property_value::download_property_value(const std::string& val)
{
    _val = std::make_shared<msdod::CDownloadPropertyValueInternal>(val);
};

download_property_value::download_property_value(uint32_t val)
{
    _val = std::make_shared<msdod::CDownloadPropertyValueInternal>(val);
};

download_property_value::download_property_value(uint64_t val)
{
    _val = std::make_shared<msdod::CDownloadPropertyValueInternal>(val);
};

download_property_value::download_property_value(bool val)
{
    _val = std::make_shared<msdod::CDownloadPropertyValueInternal>(val);
};

download_property_value::download_property_value(std::vector<unsigned char>& val)
{
    _val = std::make_shared<msdod::CDownloadPropertyValueInternal>(val);
};

download_property_value::download_property_value(const status_callback_t& val)
{
    _val = std::make_shared<msdod::CDownloadPropertyValueInternal>(val);
}

int32_t download_property_value::init_code() const noexcept
{
    return _val->init_code();
}

#if (!DO_DISABLE_EXCEPTIONS)
void download_property_value::as(bool& val) const
{
    throw_if_fail(_val->as(val));
};

void download_property_value::as(uint32_t& val) const
{
    throw_if_fail(_val->as(val));
};

void download_property_value::as(uint64_t& val) const
{
    throw_if_fail(_val->as(val));
};

void download_property_value::as(std::string& val) const
{
    throw_if_fail(_val->as(val));
};

void download_property_value::as(status_callback_t& val) const
{
    throw_if_fail(_val->as(val));
};

void download_property_value::as(std::vector<unsigned char>& val) const
{
    throw_if_fail(_val->as(val));
}
#endif // !DO_DISABLE_EXCEPTIONS

int32_t download_property_value::as_nothrow(bool& val) const noexcept
{
    return _val->as(val);
};

int32_t download_property_value::as_nothrow(uint32_t& val) const noexcept
{
    return _val->as(val);
};

int32_t download_property_value::as_nothrow(uint64_t& val) const noexcept
{
    return _val->as(val);
};

int32_t download_property_value::as_nothrow(std::string& val) const noexcept
{
    return _val->as(val);
};

int32_t download_property_value::as_nothrow(std::vector<unsigned char>& val) const noexcept
{
    return _val->as(val);
}

int32_t download_property_value::as_nothrow(status_callback_t& val) const noexcept
{
    return _val->as(val);
};

} // deliveryoptimization
} // microsoft

