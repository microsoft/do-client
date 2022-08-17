// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "do_error_helpers.h"

namespace microsoft
{
namespace deliveryoptimization
{
namespace details
{

const error_category& do_category()
{
    static error_category instance;
    return instance;
}

const char* error_category::name() const noexcept
{
    return "delivery optimization error";
}
std::string error_category::message(int32_t /*code*/) const
{
    // TODO(shishirb) describe common error codes
    return "unrecognized error";
}

} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft
