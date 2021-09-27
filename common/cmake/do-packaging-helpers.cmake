# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

macro (set_common_cpack_vars name description)
    set(CPACK_PACKAGE_NAME ${name})
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${description})
    set(CPACK_PACKAGE_VENDOR "Delivery Optimization Team")
    set(CPACK_PACKAGE_CONTACT "docloss@Microsoft.com")
    set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/microsoft/do-client")
    set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README.md")
    set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

    if (DO_PACKAGE_TYPE STREQUAL "DEB")
        set(CPACK_GENERATOR "DEB")
        set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
        set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/microsoft/do-client")
        set(CPACK_DEBIAN_PACKAGE_MAINTAINER "docloss@microsoft.com")
        # Automatically detect and enforced shared lib dependencies.
        # Note: Automatic dependency resolution requires integration with a package repository.
        set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    elseif (DO_PACKAGE_TYPE STREQUAL "RPM")
        set(CPACK_GENERATOR "RPM")
        set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
    endif ()
endmacro ()
