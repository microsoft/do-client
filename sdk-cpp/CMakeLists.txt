# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

project (deliveryoptimization_sdk VERSION 1.0.0)

if (DO_PLATFORM_LINUX)
    fixup_compile_options_for_arm()
endif()

if (DO_BUILD_TESTS)
    add_subdirectory(tests)
endif()

set(DO_SDK_LIB_NAME "deliveryoptimization")
add_do_version_lib("${DO_SDK_LIB_NAME}-lib" ${PROJECT_VERSION})
try_set_filesystem_lib()

### Region CMake Build ###

set(sdk_source_common
    "src/*.cpp"
    "src/internal/*.cpp"
)

set(sdk_public_includes
    $<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/${DO_SDK_LIB_NAME}>
)

set(sdk_private_includes_common
    "src/internal"
)

if(DO_PLATFORM_LINUX)
    include(GNUInstallDirs)

    file(GLOB sdk_source
        ${sdk_source_common}
        "src/internal/rest/*.cpp"
        "src/internal/rest/util/*.cpp"
    )

    add_library(${DO_SDK_LIB_NAME} SHARED "${sdk_source}")

    set(sdk_compile_definitions
        DOSVC_BIN_NAME="${DOSVC_BIN_NAME}"
        DO_PLUGIN_APT_BIN_NAME="${DO_PLUGIN_APT_BIN_NAME}"
    )

    set(sdk_private_includes
        "src/internal/rest"
        "src/internal/rest/util"
        ${include_directories_for_arm}
    )

    set(sdk_private_linked_libs
        doversion
        dohttp
    )

    strip_symbols(${DO_SDK_LIB_NAME})

elseif(DO_PLATFORM_MAC)
    file(GLOB sdk_source
        ${sdk_source_common}
        "src/internal/rest/*.cpp"
        "src/internal/rest/util/*.cpp"
    )

    add_library(${DO_SDK_LIB_NAME} SHARED "${sdk_source}")

    set(sdk_private_includes
        "src/internal/rest"
        "src/internal/rest/util"
    )

    set(sdk_private_linked_libs
        doversion
        dohttp
    )

elseif(DO_PLATFORM_WINDOWS)
    file(GLOB sdk_source
        ${sdk_source_common}
        "src/internal/com/*.cpp"
    )

    add_library(${DO_SDK_LIB_NAME} STATIC "${sdk_source}")

    set(sdk_private_includes
        "src/internal"
        "src/internal/com"
        "src/internal/com/deliveryoptimization"
        "src/internal/util"
    )
endif()

add_library(Microsoft::${DO_SDK_LIB_NAME} ALIAS ${DO_SDK_LIB_NAME})
target_include_directories(${DO_SDK_LIB_NAME}
    PUBLIC
        ${sdk_public_includes}
    PRIVATE
        ${sdk_private_includes}
        ${sdk_private_includes_common}
)
target_compile_definitions(${DO_SDK_LIB_NAME}
    PRIVATE
        ${sdk_compile_definitions}
)
if (DO_DEV_DEBUG)
    target_compile_definitions(${DO_SDK_LIB_NAME} PRIVATE DO_DEV_DEBUG)
endif ()
if (DO_BUILD_FOR_SNAP)
    target_compile_definitions(${DO_SDK_LIB_NAME} PRIVATE DO_BUILD_FOR_SNAP)
endif ()
if (DO_PLATFORM_LINUX OR DO_PLATFORM_MAC)
    # TODO(shishirb) Remove internal use of exceptions on these platforms
    target_compile_definitions(${DO_SDK_LIB_NAME} PRIVATE DO_ENABLE_EXCEPTIONS)
endif ()
add_platform_interface_definitions(${DO_SDK_LIB_NAME})

target_link_libraries(${DO_SDK_LIB_NAME}
    PRIVATE
        ${sdk_private_linked_libs}
        ${CXX_FILESYSTEM_LIBS}
)

### Endregion CMake Build ###

### Region CMake Install ###

if(DO_PLATFORM_LINUX)
    # Install the lib while also exporting all the usage requirements to a named target.
    install(
        TARGETS
            ${DO_SDK_LIB_NAME}
        EXPORT
            ${PROJECT_NAME}-targets
        LIBRARY
            DESTINATION
                ${CMAKE_INSTALL_LIBDIR}
            COMPONENT
                runtime
        ARCHIVE
            DESTINATION
                ${CMAKE_INSTALL_LIBDIR}
            COMPONENT
                development
    )

    # Write the usage requirements exported above to a file. This allows other cmake-aware projects to consume this library easily.
    install(
        EXPORT
            ${PROJECT_NAME}-targets
        FILE
            ${PROJECT_NAME}-targets.cmake
        NAMESPACE
            Microsoft::
        DESTINATION
            ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
        COMPONENT
            dev
    )

    # Install the public header files
    install(
        DIRECTORY
            include/
        DESTINATION
            ${CMAKE_INSTALL_INCLUDEDIR}/${DO_SDK_LIB_NAME}
        COMPONENT
            dev
        FILES_MATCHING
            PATTERN "*.h"
    )

    # Write basic version info to a cmake file
    include(CMakePackageConfigHelpers)
    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
        VERSION
            ${PROJECT_VERSION}
        COMPATIBILITY
            SameMajorVersion
    )

    # Write config files to appropriate install directory
    install(
        FILES
            ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
            ${${PROJECT_NAME}_SOURCE_DIR}/build/cmake/${PROJECT_NAME}-config.cmake
        DESTINATION
            ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
        COMPONENT
            dev
    )

    set_target_properties(${DO_SDK_LIB_NAME} PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})

    if (DO_PACKAGE_TYPE)
        message(STATUS "Packaging for ${DO_PACKAGE_TYPE}")

        # The name here is overriden by the component package names
        set_common_cpack_vars("lib${DO_SDK_LIB_NAME}"
            "The DO SDK is a Microsoft project for enabling IPC through native C++ code with the Delivery Optimization Agent for Linux")

        set(DOSDKCPP_RUNTIME_PACKAGE_NAME ${CPACK_PACKAGE_NAME})

        if (DO_PACKAGE_TYPE STREQUAL "DEB")
            set(CPACK_DEB_COMPONENT_INSTALL ON)
            set(CPACK_DEBIAN_RUNTIME_PACKAGE_NAME ${DOSDKCPP_RUNTIME_PACKAGE_NAME})

            # Automatically detect and enforced shared lib dependencies for each component
            set(CPACK_DEBIAN_DEV_PACKAGE_SHLIBDEPS ON)
            set(CPACK_DEBIAN_RUNTIME_PACKAGE_SHLIBDEPS ON)

            # Development component requires dependency on the runtime component
            # This allows installation of the development package to resolve runtime component during package resolution
            set(CPACK_DEBIAN_DEV_PACKAGE_DEPENDS ${DOSDKCPP_RUNTIME_PACKAGE_NAME})
            set(CPACK_DEBIAN_RUNTIME_PACKAGE_DEPENDS "deliveryoptimization-agent")
        elseif (DO_PACKAGE_TYPE STREQUAL "RPM")
            set(CPACK_RPM_COMPONENT_INSTALL ON)
            set(CPACK_RPM_RUNTIME_PACKAGE_NAME ${DOSDKCPP_RUNTIME_PACKAGE_NAME})

            # Automatic dependency detection is enabled by default in the rpm generator

            # Development component requires dependency on the runtime component
            # This allows installation of the development package to resolve runtime component during package resolution
            set(CPACK_RPM_DEV_PACKAGE_REQUIRES ${DOSDKCPP_RUNTIME_PACKAGE_NAME})
            set(CPACK_RPM_RUNTIME_PACKAGE_REQUIRES "deliveryoptimization-agent")
        endif()

        INCLUDE(CPack)

        # DO SDK is packaged into two components, a runtime component which includes the shared library and a development component including headers, cmake files, etc.
        # Add component needs to be after INCLUDE(CPack)
        cpack_add_component(
            runtime
            DISPLAY_NAME
                Runtime
            Description
                "Dynamic Libraries for Delivery Optimization Runtime"
            REQUIRED
        )
        cpack_add_component(
            dev
            DISPLAY_NAME
                Development
            Description
                "Headers and cmake files needed for DO SDK Development"
            REQUIRED
            DEPENDS
                runtime
        )
    endif() # Packaging
endif() # Linux

### End Region Install ###
