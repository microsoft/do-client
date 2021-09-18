
set(test_lib_name dotestutil)

add_library(${test_lib_name} STATIC do_test_helpers.cpp)
target_include_directories(${test_lib_name} 
    INTERFACE 
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${Boost_INCLUDE_DIRS} #boost/asio.hpp
)