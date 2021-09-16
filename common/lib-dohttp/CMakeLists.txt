
find_path(GSL_INCLUDE_DIR gsl)
if (GSL_INCLUDE_DIR STREQUAL "GSL_INCLUDE_DIR-NOTFOUND")
  message(FATAL_ERROR "Could not find MS Guidelines Support Library.")
endif()

add_library(dohttp STATIC do_http_parser.cpp)
target_include_directories(dohttp 
    INTERFACE 
        ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC
        ${GSL_INCLUDE_DIR})
