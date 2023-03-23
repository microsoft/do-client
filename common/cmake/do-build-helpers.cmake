# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

macro (fixup_compile_options_for_arm)
    # Special instructions for cross-compiling to arm-linux
    if (CMAKE_CXX_COMPILER MATCHES arm-linux OR CMAKE_CXX_COMPILER MATCHES aarch64-linux)
        message (STATUS "Detected ARM linux target")

        # Linux ARM cross-compiler is not picking up headers from /usr/local automatically.
        set (include_directories_for_arm
            "/usr/local/include"
            "${OPENSSL_ROOT_DIR}/include")

        # Disable incompatible ABI warnings that appear for STL calls due to ABI change in gcc 7.1
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-psabi")

        # Setup compile time definitions for static/dynamic linking of the standard c/c++ libs
        set (STATIC_CXX_RUNTIME_FLAG "-static-libstdc++")
        set (STATIC_CXX_RUNTIME_FLAG_MATCH "\\\-static\\\-libstdc\\\+\\\+")
        set (DYNAMIC_CXX_RUNTIME_FLAG "-lstdc++")
        set (DYNAMIC_CXX_RUNTIME_FLAG_MATCH "\\\-lstdc\\\+\\\+")

        if (USE_STATIC_CXX_RUNTIME)
            set (DESIRED_CXX_RUNTIME_FLAG ${STATIC_CXX_RUNTIME_FLAG})
            set (REPLACE_CXX_RUNTIME_FLAG ${DYNAMIC_CXX_RUNTIME_FLAG_MATCH})
        else ()
            set (DESIRED_CXX_RUNTIME_FLAG ${DYNAMIC_CXX_RUNTIME_FLAG})
            set (REPLACE_CXX_RUNTIME_FLAG ${STATIC_CXX_RUNTIME_FLAG_MATCH})
        endif ()

        set (cxx_variables
            CMAKE_CXX_FLAGS_DEBUG
            CMAKE_CXX_FLAGS_MINSIZEREL
            CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_RELWITHDEBINFO)

        # Replace the cxx compiler options
        foreach (variable ${cxx_variables})
            if (${variable} MATCHES "${REPLACE_CXX_RUNTIME_FLAG}")
                string (REGEX
                        REPLACE ${REPLACE_CXX_RUNTIME_FLAG}
                                ${DESIRED_CXX_RUNTIME_FLAG}
                                ${variable}
                                "${${variable}}")
            else ()
                set (${variable} "${${variable}} ${DESIRED_CXX_RUNTIME_FLAG}")
            endif ()
        endforeach ()

        message (STATUS "C compiler: ${CMAKE_C_COMPILER}")
        message (STATUS "CXX compiler: ${CMAKE_CXX_COMPILER}")
    endif ()
endmacro ()

# Credit: https://github.com/Azure/adu-private-preview/blob/master/src/agent/CMakeLists.txt
function (add_gitinfo_definitions target_name)

    # Pick up Git revision so we can report it in version information.

    include (FindGit)
    if (GIT_FOUND)
        execute_process (
            COMMAND ${GIT_EXECUTABLE} rev-parse --show-toplevel
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_ROOT
            OUTPUT_STRIP_TRAILING_WHITESPACE)
    endif ()
    if (GIT_ROOT)
        execute_process (
            COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            OUTPUT_VARIABLE DO_GIT_HEAD_REVISION OUTPUT_STRIP_TRAILING_WHITESPACE)
        execute_process (
            COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            OUTPUT_VARIABLE DO_GIT_HEAD_NAME OUTPUT_STRIP_TRAILING_WHITESPACE)

        target_compile_definitions (${target_name}
            PRIVATE
                DO_VER_GIT_HEAD_NAME="${DO_GIT_HEAD_NAME}"
                DO_VER_GIT_HEAD_REVISION="${DO_GIT_HEAD_REVISION}")
    else ()
        message (WARNING "Git version info not found, DO NOT release from this build tree!")
        target_compile_definitions (${target_name}
            PRIVATE
                DO_VER_GIT_HEAD_NAME=""
                DO_VER_GIT_HEAD_REVISION="")
    endif ()

endfunction ()

function (add_component_version_definitions target_name component_name maj_min_patch_ver)
    target_compile_definitions (${target_name}
        PRIVATE
            DO_VER_BUILDER_IDENTIFIER="${DO_BUILDER_IDENTIFIER}"
            DO_VER_BUILD_TIME="${DO_BUILD_TIMESTAMP}"
            DO_VER_COMPONENT_NAME="${component_name}"
            DO_VER_COMPONENT_VERSION="${maj_min_patch_ver}")

    add_gitinfo_definitions (${target_name})
endfunction ()

macro (add_do_version_lib target_name maj_min_patch_ver)
    set(DO_COMPONENT_NAME "${target_name}")
    set(DO_COMPONENT_VERSION "${maj_min_patch_ver}")

    # CMake requires us to specify the binary dir when the source dir is not a child of the current dir
    add_subdirectory(${do_project_root_SOURCE_DIR}/common ${CMAKE_CURRENT_BINARY_DIR}/common)
endmacro ()

# From https://github.com/dotnet/coreclr/pull/3872/files
# For minsizerel build, split unneeded symbols from target binary file and into a separate .dbg file.
# Reduces the installed size on disk of our binaries while still making symbols available for debugging.
function(strip_symbols targetName)
    string(TOLOWER ${CMAKE_BUILD_TYPE} MY_BUILD_TYPE)
    if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux" AND MY_BUILD_TYPE STREQUAL minsizerel)

        find_program(OBJCOPY objcopy)
        if (OBJCOPY STREQUAL "OBJCOPY-NOTFOUND")
            message(WARNING "objcopy not found. Binaries will not be stripped.")
        else()
            set(strip_source_file $<TARGET_FILE:${targetName}>)
            set(strip_destination_file ${strip_source_file}.dbg)
            add_custom_command(
                TARGET ${targetName}
                POST_BUILD
                VERBATIM
                COMMAND ${OBJCOPY} --only-keep-debug ${strip_source_file} ${strip_destination_file}
                COMMAND ${OBJCOPY} --strip-unneeded ${strip_source_file}
                COMMAND ${OBJCOPY} --add-gnu-debuglink=${strip_destination_file} ${strip_source_file}
                COMMENT Stripping symbols from ${strip_source_file} into file ${strip_destination_file})
        endif()

    endif()
endfunction()

function (add_platform_interface_definitions target_name)
    if (DO_PLATFORM_LINUX)
        target_compile_definitions (${target_name} PRIVATE DO_PLATFORM_LINUX=1)
    endif ()
    if (DO_PLATFORM_WINDOWS)
        target_compile_definitions (${target_name} PRIVATE DO_PLATFORM_WINDOWS=1)
    endif ()
    if (DO_PLATFORM_MAC)
        target_compile_definitions (${target_name} PRIVATE DO_PLATFORM_MAC=1)
    endif ()

    if (DO_INTERFACE_REST)
        target_compile_definitions (${target_name} PRIVATE DO_INTERFACE_REST=1)
    endif ()
    if (DO_INTERFACE_COM)
        target_compile_definitions (${target_name} PRIVATE DO_INTERFACE_COM=1)
    endif ()

    if (DO_CLIENT_AGENT)
        target_compile_definitions (${target_name} PRIVATE DO_CLIENT_AGENT=1)
    endif ()
    if (DO_CLIENT_DOSVC)
        target_compile_definitions (${target_name} PRIVATE DO_CLIENT_DOSVC=1)
    endif ()

endfunction ()

function (try_set_filesystem_lib)

    # Sets the variable CXX_FILESYSTEM_LIBS if an extra lib is required for c++ filesystem support.
    # C++17 std::filesystem support is implemented in std::experimental::filesystem prior to GCC 9.
    # Only the experimental version requires linking to stdc++fs library (so troublesome!).
    # There is no built-in support in cmake to handle this difference (like a FindFileSystem.cmake module).
    # See also: Portable linking for C++17 std::filesystem (https://gitlab.kitware.com/cmake/cmake/-/issues/17834)

    if (DO_PLATFORM_WINDOWS)
        # std::filesystem not required for Windows builds
        return ()
    endif ()

    message ("Compiler identified: ${CMAKE_CXX_COMPILER_ID} - ${CMAKE_CXX_COMPILER_VERSION}")
    if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
        if (${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 9.0.0)
            message (STATUS "Using std::experimental filesystem library")
            set(CXX_FILESYSTEM_LIBS stdc++fs PARENT_SCOPE)
        endif ()
    endif ()

endfunction ()
