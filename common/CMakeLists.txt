if (DO_BUILD_TESTS)
    # Plugins subproject doesn't have test code
    if (DO_INCLUDE_AGENT OR DO_INCLUDE_SDK)
        add_subdirectory(lib-dotestutil)
    endif()
      


endif()

add_subdirectory(lib-doversion)
