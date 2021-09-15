if (${DO_PLATFORM_ID} EQUAL ${DO_PLATFORM_ID_LINUX} OR ${DO_PLATFORM_ID} EQUAL ${DO_PLATFORM_ID_MAC})
    add_subdirectory(lib-dohttp)
    if (DO_BUILD_TESTS)
        # Plugins subproject doesn't have test code
        if (DO_INCLUDE_AGENT OR DO_INCLUDE_SDK)
            add_subdirectory(lib-dotestutil)
        endif()
    endif()
endif()

add_subdirectory(lib-doversion)
