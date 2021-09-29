#ifndef _DELIVERY_OPTIMIZATION_DO_DOWNLOAD_PROPERTY_INTERNAL_H
#define _DELIVERY_OPTIMIZATION_DO_DOWNLOAD_PROPERTY_INTERNAL_H

#if defined(DO_INTERFACE_COM)
#include <OAIdl.h>
#else
#include <boost/variant.hpp>
#endif

#include <functional>
#include <string>
#include <vector>

#include "do_download_property.h"

namespace microsoft
{
namespace deliveryoptimization
{
namespace details
{
class CDownloadPropertyValueInternal
{
public:
#if defined(DO_INTERFACE_COM)
    using native_type = VARIANT;
#else
    //TODO(jimson): Not sure yet if boost::variant is the right solution here, for now anyways all constructors will throw for REST side
    using native_type = boost::variant<std::string, uint32_t, uint64_t, bool, std::vector<unsigned char>>;
#endif

    CDownloadPropertyValueInternal() = default;

    explicit CDownloadPropertyValueInternal(const std::string& val);
    explicit CDownloadPropertyValueInternal(uint32_t val);
    explicit CDownloadPropertyValueInternal(uint64_t val);
    explicit CDownloadPropertyValueInternal(bool val);
    explicit CDownloadPropertyValueInternal(std::vector<unsigned char>& val); // DODownloadProperty_SecurityContext

    explicit CDownloadPropertyValueInternal(const download_property_value::status_callback_t& val);

    ~CDownloadPropertyValueInternal();

    CDownloadPropertyValueInternal(const CDownloadPropertyValueInternal& rhs);
    CDownloadPropertyValueInternal& operator=(CDownloadPropertyValueInternal copy);
    CDownloadPropertyValueInternal(CDownloadPropertyValueInternal&& rhs) noexcept;

    friend void swap(CDownloadPropertyValueInternal& first, CDownloadPropertyValueInternal& second) noexcept
    {
        std::swap(first._var, second._var);
        std::swap(first._callback, second._callback);
    }

    void as(bool& val) const;
    void as(uint32_t& val) const;
    void as(uint64_t& val) const;
    void as(std::string& val) const;
    void as(std::vector<unsigned char>& val) const;
    void as(download_property_value::status_callback_t& val) const noexcept;
    
    const native_type& native_value() const noexcept;

private:
    native_type _var;
    download_property_value::status_callback_t _callback;
};
} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft

#endif // _DELIVERY_OPTIMIZATION_DO_DOWNLOAD_PROPERTY_H
