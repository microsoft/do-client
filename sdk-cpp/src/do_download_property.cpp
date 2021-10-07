// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "do_download_property.h"

#include "do_download_property_internal.h"
#include "do_errors.h"

namespace msdod = microsoft::deliveryoptimization::details;

namespace microsoft
{
namespace deliveryoptimization
{

download_property_value::download_property_value()
{
    _val = std::make_shared<msdod::CDownloadPropertyValueInternal>();
}

#if (DO_ENABLE_EXCEPTIONS)
download_property_value download_property_value::make(const std::string& val)
{
    download_property_value out;
    throw_if_fail(make_nothrow(out, val));
    return out;
}

download_property_value download_property_value::make(uint32_t val)
{
    download_property_value out;
    throw_if_fail(make_nothrow(out, val));
    return out;
}

download_property_value download_property_value::make(uint64_t val)
{
    download_property_value out;
    throw_if_fail(make_nothrow(out, val));
    return out;
}

download_property_value download_property_value::make(bool val)
{
    download_property_value out;
    throw_if_fail(make_nothrow(out, val));
    return out;
}

download_property_value download_property_value::make(std::vector<unsigned char>& val)
{
    download_property_value out;
    throw_if_fail(make_nothrow(out, val));
    return out;
}

download_property_value download_property_value::make(const status_callback_t& val)
{
    download_property_value out;
    throw_if_fail(make_nothrow(out, val));
    return out;
}

void download_property_value::as(bool& val) const
{
    throw_if_fail(_val->As(val));
};

void download_property_value::as(uint32_t& val) const
{
    throw_if_fail(_val->As(val));
};

void download_property_value::as(uint64_t& val) const
{
    throw_if_fail(_val->As(val));
};

void download_property_value::as(std::string& val) const
{
    throw_if_fail(_val->As(val));
};

void download_property_value::as(status_callback_t& val) const
{
    throw_if_fail(_val->As(val));
};

void download_property_value::as(std::vector<unsigned char>& val) const
{
    throw_if_fail(_val->As(val));
}
#endif

int32_t download_property_value::make_nothrow(download_property_value& out, const std::string& val)
{
    download_property_value temp;
    RETURN_IF_FAILED(temp._val->Init(val));

    out = temp;
    return S_OK;;
}

int32_t download_property_value::make_nothrow(download_property_value& out, uint32_t val)
{
    download_property_value temp;
    RETURN_IF_FAILED(temp._val->Init(val));

    out = temp;
    return S_OK;;
}

int32_t download_property_value::make_nothrow(download_property_value& out, uint64_t val)
{
    download_property_value temp;
    RETURN_IF_FAILED(temp._val->Init(val));

    out = temp;
    return S_OK;;
}

int32_t download_property_value::make_nothrow(download_property_value& out, bool val)
{
    download_property_value temp;
    RETURN_IF_FAILED(temp._val->Init(val));

    out = temp;
    return S_OK;;
}

int32_t download_property_value::make_nothrow(download_property_value& out, std::vector<unsigned char>& val)
{
    download_property_value temp;
    RETURN_IF_FAILED(temp._val->Init(val));

    out = temp;
    return S_OK;;
}

int32_t download_property_value::make_nothrow(download_property_value& out, const status_callback_t& val)
{
    download_property_value temp;
    RETURN_IF_FAILED(temp._val->Init(val));

    out = temp;
    return S_OK;;
}

int32_t download_property_value::as_nothrow(bool& val) const noexcept
{
    return _val->As(val);
};

int32_t download_property_value::as_nothrow(uint32_t& val) const noexcept
{
    return _val->As(val);
};

int32_t download_property_value::as_nothrow(uint64_t& val) const noexcept
{
    return _val->As(val);
};

int32_t download_property_value::as_nothrow(std::string& val) const noexcept
{
    return _val->As(val);
};

int32_t download_property_value::as_nothrow(std::vector<unsigned char>& val) const noexcept
{
    return _val->As(val);
}

int32_t download_property_value::as_nothrow(status_callback_t& val) const noexcept
{
    return _val->As(val);
};

} // deliveryoptimization
} // microsoft

