#pragma once

#include <string>

namespace microsoft
{
namespace deliveryoptimization
{
namespace util
{
namespace details
{

const char* SimpleVersion();
std::string ComponentVersion(bool fIncludeExtras = true);
bool OutputVersionIfNeeded(int argc, char** argv);

}
}
}
} // namespace microsoft::deliveryoptimization::util::details
