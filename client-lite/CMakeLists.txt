if (NOT DOSVC_BIN_NAME)
    message (FATAL_ERROR "Agent daemon name not defined")
endif ()

project (${DOSVC_BIN_NAME} VERSION 0.5.0)

option (DO_PROXY_SUPPORT "Set DO_PROXY_SUPPORT to OFF to turn off proxy support for downloads and thus remove dependency on libproxy." ON)

add_definitions(-DBOOST_ALL_DYN_LINK=1)

# Get full debug info and also define DEBUG macro in debug builds
string(TOLOWER ${CMAKE_BUILD_TYPE} DO_BUILD_TYPE)
if (DO_BUILD_TYPE MATCHES debug)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")
endif()

# -Wno-noexcept-type, the offending function is SetResultLoggingCallback, this warning is fixed in C++17 because exception specification
# is part of a function type. Since the offending function is not public when compiled into docs_common just add the compiler flag here
# to disable the warning.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-noexcept-type")

fixup_compile_options_for_arm()

function (target_link_dl_lib target)
    if (CMAKE_CXX_COMPILER MATCHES arm-linux OR CMAKE_CXX_COMPILER MATCHES aarch64-linux)
        # Required for static linking openssl on peabody.
        # Provides dynamic library functions like dlopen.
        find_library (DL_LIB NAMES dl)
        message("Found DL Lib for ${target}: ${DL_LIB}")
        target_link_libraries(${target} ${DL_LIB})
    endif ()
endfunction ()

# Include external libraries here:
find_package(Boost COMPONENTS filesystem REQUIRED)

# Cpprest Issues:
# 1. v2.10.10 min version (see required PR link below). cpprestsdk does not seem to support specifying this through cmake.
# https://github.com/microsoft/cpprestsdk/pull/1019/files
#
# 2. Installing libcpprest-dev via apt installs cpprest's cmake config files to a non-default cmake search path before v2.10.9
# See: https://github.com/microsoft/cpprestsdk/issues/686
# This issue has been patched but has not made its way to the stable branch for Ubuntu
# Since we are statically linking to v2.10.16 we no longer need to worry about the above as cpprest is patched to provide the proper package configuration metadata
find_package(cpprestsdk CONFIG REQUIRED)

if (DO_PROXY_SUPPORT)
    message (STATUS "Proxy support requested. Will look for libproxy.")
    find_package(libproxy REQUIRED)
else ()
    message (STATUS "Proxy support not requested.")
endif ()

find_path(GSL_INCLUDE_DIR gsl)
if (GSL_INCLUDE_DIR STREQUAL "GSL_INCLUDE_DIR-NOTFOUND")
  message(FATAL_ERROR "Could not find MS Guidelines Support Library.")
endif()

set (docs_common_includes
    ${include_directories_for_arm}
    ${GSL_INCLUDE_DIR}
    "${PROJECT_SOURCE_DIR}/src/config"
    "${PROJECT_SOURCE_DIR}/src/download"
    "${PROJECT_SOURCE_DIR}/src/include"
    "${PROJECT_SOURCE_DIR}/src/ipc"
    "${PROJECT_SOURCE_DIR}/src/threading"
    "${PROJECT_SOURCE_DIR}/src/trace"
    "${PROJECT_SOURCE_DIR}/src/util"
)

# Please maintain this list in alphabetical order.
file (GLOB files_docs_common
    src/config/*.cpp
    src/download/*.cpp
    src/ipc/*.cpp
    src/threading/*.cpp
    src/trace/*.cpp
    src/util/*.cpp
)

# Dir path used for docs_common and debian control scripts
set(docs_svc_config_dir_path "/etc/${DOSVC_BIN_NAME}")
set(docs_svc_log_dir_path "/var/log/${DOSVC_BIN_NAME}")
set(docs_svc_run_dir_path "/var/run/${DOSVC_BIN_NAME}")

# Build product files into a lib for use by other targets
add_library(docs_common STATIC ${files_docs_common})
target_compile_definitions(docs_common
    PRIVATE DO_CONFIG_DIRECTORY_PATH="${docs_svc_config_dir_path}"
            DO_AGENT_LOG_DIRECTORY_PATH="${docs_svc_log_dir_path}"
            DO_RUN_DIRECTORY_PATH="${docs_svc_run_dir_path}"
)

target_include_directories(docs_common PUBLIC ${docs_common_includes})

if (DO_PROXY_SUPPORT)
    target_compile_definitions(docs_common PRIVATE DO_PROXY_SUPPORT)
    target_link_libraries(docs_common PUBLIC libproxy::proxy)
endif ()

target_link_dl_lib(docs_common)

# Include excutables entry point
file (GLOB files_docs_exe
    src/exe/*.cpp
)

add_do_version_lib(${PROJECT_NAME} ${PROJECT_VERSION})

# Add the executable
add_executable(${DOSVC_BIN_NAME} ${files_docs_exe})
target_include_directories(${DOSVC_BIN_NAME} PRIVATE ${docs_common_includes})
target_link_libraries(${DOSVC_BIN_NAME}
    doversion
    docs_common
    cpprestsdk::cpprest
    ${Boost_LIBRARIES})

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    strip_symbols(${DOSVC_BIN_NAME})

    include(GNUInstallDirs)

    # Install binary
    set(DO_INSTALL_BINDIR ${CMAKE_INSTALL_FULL_BINDIR})
    install(TARGETS ${DOSVC_BIN_NAME} DESTINATION ${DO_INSTALL_BINDIR})

    if(DO_PACKAGE_TYPE)
        message(STATUS "Packaging for ${DO_PACKAGE_TYPE}")

        set_common_cpack_vars(${DOSVC_BIN_NAME} "Delivery Optimization downloader with Microsoft Connected Cache support")

        # Packaging scripts and variables required for them
        set(docs_svc_name "${DOSVC_BIN_NAME}.service")
        set(docs_svc_bin_path ${DO_INSTALL_BINDIR}/${DOSVC_BIN_NAME})
        set(docs_systemd_cfg_path "/etc/systemd/system/${docs_svc_name}")
        set(do_user "do")
        set(do_group "do")

        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/build/postinst.in.sh
                        ${CMAKE_CURRENT_BINARY_DIR}/packaging/postinst @ONLY)

        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/build/prerm.in.sh
                        ${CMAKE_CURRENT_BINARY_DIR}/packaging/prerm @ONLY)
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/build/postrm.in.sh
                        ${CMAKE_CURRENT_BINARY_DIR}/packaging/postrm @ONLY)

        if (DO_PACKAGE_TYPE STREQUAL "DEB")
            # Install/remove scripts
            set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_BINARY_DIR}/packaging/postinst;${CMAKE_CURRENT_BINARY_DIR}/packaging/prerm;${CMAKE_CURRENT_BINARY_DIR}/packaging/postrm")
        elseif (DO_PACKAGE_TYPE STREQUAL "RPM")
            # Install/Uninstall scripts
            set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE ${CMAKE_CURRENT_BINARY_DIR}/packaging/postinst)
            set(CPACK_RPM_PRE_UNINSTALL_SCRIPT_FILE ${CMAKE_CURRENT_BINARY_DIR}/packaging/prerm)
            set(CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE ${CMAKE_CURRENT_BINARY_DIR}/packaging/postrm)
            # Automatic dependency detection is enabled by default in the rpm generator
        endif()

        include(CPack)
    endif()

endif() # Linux

if(DO_BUILD_TESTS)
    add_subdirectory (test)
endif()