#pragma once

#include <string>

namespace microsoft::deliveryoptimization::util::details
{

const char* SimpleVersion();
std::string ComponentVersion(bool fIncludeExtras = true);
bool OutputVersionIfNeeded(int argc, char** argv);

}
