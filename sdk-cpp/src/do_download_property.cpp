// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "do_download_property.h"

#include "do_download_property_internal.h"

#include "do_exceptions.h"

namespace msdod = microsoft::deliveryoptimization::details;

namespace microsoft
{
namespace deliveryoptimization
{

#if (DO_ENABLE_EXCEPTIONS)
void download_property_value::make(const std::string& val)
{
    _val = std::make_shared<msdod::CDownloadPropertyValueInternal>(val);
    throw_if_fail(_val->init_code());
}

void download_property_value::make(uint32_t val)
{
    _val = std::make_shared<msdod::CDownloadPropertyValueInternal>(val);
    throw_if_fail(_val->init_code());
}

void download_property_value::make(uint64_t val)
{
    _val = std::make_shared<msdod::CDownloadPropertyValueInternal>(val);
    throw_if_fail(_val->init_code());;
}

void download_property_value::make(bool val)
{
    _val = std::make_shared<msdod::CDownloadPropertyValueInternal>(val);
    throw_if_fail(_val->init_code());
}

void download_property_value::make(std::vector<unsigned char>& val)
{
    _val = std::make_shared<msdod::CDownloadPropertyValueInternal>(val);
    throw_if_fail(_val->init_code());
}

void download_property_value::make(const status_callback_t& val)
{
    _val = std::make_shared<msdod::CDownloadPropertyValueInternal>(val);
    throw_if_fail(_val->init_code());
}

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

#endif

int32_t download_property_value::make_nothrow(const std::string& val)
{
    _val = std::make_shared<msdod::CDownloadPropertyValueInternal>(val);
    return _val->init_code();
}

int32_t download_property_value::make_nothrow(uint32_t val)
{
    _val = std::make_shared<msdod::CDownloadPropertyValueInternal>(val);
    return _val->init_code();
}

int32_t download_property_value::make_nothrow(uint64_t val)
{
    _val = std::make_shared<msdod::CDownloadPropertyValueInternal>(val);
    return _val->init_code();
}

int32_t download_property_value::make_nothrow(bool val)
{
    _val = std::make_shared<msdod::CDownloadPropertyValueInternal>(val);
    return _val->init_code();
}

int32_t download_property_value::make_nothrow(std::vector<unsigned char>& val)
{
    _val = std::make_shared<msdod::CDownloadPropertyValueInternal>(val);
    return _val->init_code();
}

int32_t download_property_value::make_nothrow(const status_callback_t& val)
{
    _val = std::make_shared<msdod::CDownloadPropertyValueInternal>(val);
    return _val->init_code();
}

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

