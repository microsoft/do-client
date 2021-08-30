#ifndef _DELIVERY_OPTIMIZATION_DO_CPPREST_UTILS_H
#define _DELIVERY_OPTIMIZATION_DO_CPPREST_UTILS_H

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace microsoft
{
namespace deliveryoptimization
{
namespace details
{
namespace cpprest_utils
{

#define _ASYNCRTIMP
#ifndef CPPREST_NOEXCEPT
#define CPPREST_NOEXCEPT noexcept
#endif
#ifndef _XPLATSTR
// All strings are narrow for DO
typedef char char_t;
typedef std::string string_t;
#define _XPLATSTR(x) x
typedef std::ostringstream ostringstream_t;
typedef std::ofstream ofstream_t;
typedef std::ostream ostream_t;
typedef std::istream istream_t;
typedef std::ifstream ifstream_t;
typedef std::istringstream istringstream_t;
typedef std::stringstream stringstream_t;
#define ucout std::cout
#define ucin std::cin
#define ucerr std::cerr
#endif

} // namespace cpprest_utils
} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft

#endif // _DELIVERY_OPTIMIZATION_DO_CPPREST_UTILS_H
