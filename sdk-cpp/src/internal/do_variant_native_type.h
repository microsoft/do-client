#ifndef _DELIVERY_OPTIMIZATION_DO_VARIANT_NATIVE_TYPE_H
#define _DELIVERY_OPTIMIZATION_DO_VARIANT_NATIVE_TYPE_H

#if defined(DO_INTERFACE_COM)
#include <OAIdl.h>
#else
#include <boost/variant.hpp>
#endif

namespace microsoft
{
namespace deliveryoptimization
{
#if defined(DO_INTERFACE_COM)
using variant_native_type = VARIANT;
#else
using native_type = boost::variant<std::string, uint32_t, uint64_t, bool, std::vector<unsigned char>>;
#endif
}
}
#endif // _DELIVERY_OPTIMIZATION_DO_VARIANT_NATIVE_TYPE_H
