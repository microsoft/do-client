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

void download_property_value::as(bool& val) const
{
    _val->as(val);
};

void download_property_value::as(uint32_t& val) const
{
    _val->as(val);
};

void download_property_value::as(uint64_t& val) const
{
    _val->as(val);
};

void download_property_value::as(std::string& val) const
{
    _val->as(val);
};

void download_property_value::as(status_callback_t& val) const
{
    _val->as(val);
};

void download_property_value::as(std::vector<unsigned char>& val) const
{
    _val->as(val);
}

} // deliveryoptimization
} // microsoft

