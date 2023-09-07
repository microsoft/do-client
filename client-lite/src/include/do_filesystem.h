// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _DELIVERY_OPTIMIZATION_DO_FILESYSTEM_H
#define _DELIVERY_OPTIMIZATION_DO_FILESYSTEM_H

#if defined(__cpp_lib_filesystem)
#define INCLUDE_STD_FILESYSTEM_EXPERIMENTAL 0
#elif defined(__cpp_lib_experimental_filesystem)
#define INCLUDE_STD_FILESYSTEM_EXPERIMENTAL 1
#elif !defined(__has_include)
// Cannot check if headers exist, assume experimental
#define INCLUDE_STD_FILESYSTEM_EXPERIMENTAL 1
#elif __has_include(<filesystem>)
#define INCLUDE_STD_FILESYSTEM_EXPERIMENTAL 0
#elif __has_include(<experimental/filesystem>)
#define INCLUDE_STD_FILESYSTEM_EXPERIMENTAL 1
#else
#error Could not find system header "<filesystem>" or "<experimental/filesystem>"
#endif

#if INCLUDE_STD_FILESYSTEM_EXPERIMENTAL
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

#endif // _DELIVERY_OPTIMIZATION_DO_FILESYSTEM_H
