project (deliveryoptimization_sdk VERSION 0.4.0)

if (CMAKE_SYSTEM_NAME MATCHES Linux)
    message("DO configured to use shared Boost libraries")

    add_definitions(-DBOOST_ALL_DYN_LINK=1)
endif()

# Differentiate cross platform builds
# Note: The C preprocessor can compare integer values but not strings
set(DO_PLATFORM_ID_UNKNOWN 0)
set(DO_PLATFORM_ID_LINUX 1)
set(DO_PLATFORM_ID_MAC 2)
set(DO_PLATFORM_ID_WINDOWS 3)
if (CMAKE_SYSTEM_NAME MATCHES Linux)
    set(DO_PLATFORM_ID ${DO_PLATFORM_ID_LINUX})
elseif (CMAKE_SYSTEM_NAME MATCHES Windows)
    message(WARNING "Currently unsupported platform")
    set(DO_PLATFORM_ID ${DO_PLATFORM_ID_WINDOWS})
elseif (CMAKE_SYSTEM_NAME MATCHES Darwin) # Mac
    message(WARNING "Currently unsupported platform")
    set(DO_PLATFORM_ID ${DO_PLATFORM_ID_MAC})
else()
    message(WARNING "Unknown platform")
    set(DO_PLATFORM_ID ${DO_PLATFORM_ID_UNKNOWN})
endif()

if (CMAKE_SYSTEM_NAME MATCHES Linux)
    fixup_compile_options_for_arm()
endif()

# Include external libraries here
find_package(Boost COMPONENTS filesystem system REQUIRED)

# Cpprest Issues (Linux Only):
# 1. v2.10.10 min version (see required PR link below). cpprestsdk does not seem to support specifying this through cmake.
# https://github.com/microsoft/cpprestsdk/pull/1019/files
#
# 2. Installing libcpprest-dev via apt installs cpprest's cmake config files to a non-default cmake search path before v2.10.9
# See: https://github.com/microsoft/cpprestsdk/issues/686
# This issue has been patched but has not made its way to the stable branch for Ubuntu
# Since we are statically linking to v2.10.16 we no longer need to worry about the above as cpprest is patched to provide the proper package configuration metadata
find_package(cpprestsdk CONFIG REQUIRED)

if (DO_BUILD_TESTS)
    add_subdirectory(tests)
endif()

set(DO_SDK_LIB_NAME "deliveryoptimization")

add_do_version_lib("${DO_SDK_LIB_NAME}-lib" ${PROJECT_VERSION})

if(${DO_PLATFORM_ID} MATCHES ${DO_PLATFORM_ID_LINUX})
    include(GNUInstallDirs)

    file(GLOB sdk_source
    "src/*.cpp"
    "src/internal/rest/*.cpp"
    "src/internal/rest/util/*.cpp"
    "src/internal/common/*.cpp"
    "src/internal/common/util/*.cpp"
    )

    add_library(${DO_SDK_LIB_NAME} SHARED "${sdk_source}")
    add_library(Microsoft::${DO_SDK_LIB_NAME} ALIAS ${DO_SDK_LIB_NAME})
    target_include_directories(${DO_SDK_LIB_NAME}
        PUBLIC
            $<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include/${DO_SDK_LIB_NAME}>
        PRIVATE
            "src/internal"
            "src/internal/rest"
            "src/internal/rest/util"
            "src/internal/common"
            "src/internal/common/util"
            ${docs_common_includes}
            ${include_directories_for_arm}
    )
    target_compile_definitions(${DO_SDK_LIB_NAME} PRIVATE
        DO_PLATFORM_ID=${DO_PLATFORM_ID}
        DO_PLATFORM_ID_UNKNOWN=${DO_PLATFORM_ID_UNKNOWN}
        DO_PLATFORM_ID_LINUX=${DO_PLATFORM_ID_LINUX}
        DO_PLATFORM_ID_MAC=${DO_PLATFORM_ID_MAC}
        DO_PLATFORM_ID_WINDOWS=${DO_PLATFORM_ID_WINDOWS}
        DOSVC_BIN_NAME="${DOSVC_BIN_NAME}"
        DO_PLUGIN_APT_BIN_NAME="${DO_PLUGIN_APT_BIN_NAME}"
    )
    target_link_libraries(${DO_SDK_LIB_NAME}
        PRIVATE doversion cpprestsdk::cpprest
        PUBLIC ${Boost_LIBRARIES}
    )

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

    endif() # Build for Packaging

endif() # Linux