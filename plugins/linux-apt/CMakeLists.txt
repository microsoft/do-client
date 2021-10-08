# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

option (DO_ENABLE_EXCEPTIONS "Set DO_ENABLE_EXCEPTIONS to ON to disable compiling with exceptions" ON)

if (NOT DO_PLUGIN_APT_BIN_NAME)
    message (FATAL_ERROR "APT plugin binary name not defined")
endif ()

project (${DO_PLUGIN_APT_BIN_NAME} VERSION 0.5.0)

find_package(deliveryoptimization_sdk CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(UUID REQUIRED)

file(GLOB dopapt_source
    "*.cpp"
)

set(dopapt_cache_path "/var/cache/deliveryoptimization-plugin-apt")
set(dopapt_log_path "/tmp/deliveryoptimization-plugin-apt.log")

add_executable(${DO_PLUGIN_APT_BIN_NAME} "${dopapt_source}")
target_compile_definitions(${DO_PLUGIN_APT_BIN_NAME}
    PRIVATE
        DO_PLUGIN_APT_CACHE_PATH="${dopapt_cache_path}"
        DO_PLUGIN_APT_LOG_PATH="${dopapt_log_path}"
        DO_ENABLE_EXCEPTIONS
)

add_platform_interface_definitions(${DO_PLUGIN_APT_BIN_NAME})
add_do_version_lib(${DO_PLUGIN_APT_BIN_NAME} ${PROJECT_VERSION})

target_link_libraries(${DO_PLUGIN_APT_BIN_NAME} PRIVATE
    doversion
    Microsoft::deliveryoptimization
    OpenSSL::Crypto
    UUID::UUID
)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    strip_symbols(${DO_PLUGIN_APT_BIN_NAME})

    include(GNUInstallDirs)
    set(dopapt_install_dir ${CMAKE_INSTALL_FULL_BINDIR})
    install(TARGETS ${DO_PLUGIN_APT_BIN_NAME} DESTINATION ${dopapt_install_dir})

    if(DO_PACKAGE_TYPE)
        message(STATUS "Packaging for ${DO_PACKAGE_TYPE}")

        # Set variables used in the control scripts
        set(apt_methods_root "/usr/lib/apt/methods")
        set(dopapt_bin_path "${dopapt_install_dir}/${DO_PLUGIN_APT_BIN_NAME}")
        set(do_user_group "do")

        # Debian control scripts
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/postinst.in.sh
                        ${CMAKE_CURRENT_BINARY_DIR}/packaging/postinst @ONLY)
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/prerm.in.sh
                        ${CMAKE_CURRENT_BINARY_DIR}/packaging/prerm @ONLY)
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/postrm.in.sh
                        ${CMAKE_CURRENT_BINARY_DIR}/packaging/postrm @ONLY)

        set_common_cpack_vars(${DO_PLUGIN_APT_BIN_NAME} "Microsoft project that enables APT downloads to go through the Delivery Optimization Agent")

        if(DO_PACKAGE_TYPE STREQUAL "DEB")
            set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_BINARY_DIR}/packaging/postinst;${CMAKE_CURRENT_BINARY_DIR}/packaging/prerm;${CMAKE_CURRENT_BINARY_DIR}/packaging/postrm;")

            # Note: Dependency resolution will not work without integration into a debian package repository, until then packages must be
            # installed manually in order of dependency tree.
            set(CPACK_DEBIAN_PACKAGE_DEPENDS "libdeliveryoptimization")
        elseif (DO_PACKAGE_TYPE STREQUAL "RPM")
            set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE ${CMAKE_CURRENT_BINARY_DIR}/packaging/postinst)
            set(CPACK_RPM_PRE_UNINSTALL_SCRIPT_FILE ${CMAKE_CURRENT_BINARY_DIR}/packaging/prerm)
            set(CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE ${CMAKE_CURRENT_BINARY_DIR}/packaging/postrm)

            # Automatic dependency detection is enabled by default in the rpm generator

            # Note: Dependency resolution will not work without integration into rpm package repository
            set(CPACK_RPM_PACKAGE_REQUIRES "libdeliveryoptimization")
        endif()
        include(CPack)
    endif()
endif()
