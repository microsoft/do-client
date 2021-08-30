
add_library(dohttp STATIC
    do_cpprest_uri_builder.cpp
    do_cpprest_uri.cpp
    do_cpprest_utils.cpp
    do_http_defines.cpp
    do_http_parser.cpp
    )
target_include_directories(dohttp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
