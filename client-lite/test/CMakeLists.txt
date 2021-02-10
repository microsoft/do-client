# Unit tests for DOCS

# Only need program_options here but have to include others required by docs_common.
# TODO: docs_common should declare what it needs via target_link_libraries(PUBLIC).
find_package(Boost COMPONENTS log filesystem program_options REQUIRED)
find_package(GTest REQUIRED)

file (GLOB files_docs_tests
    *.cpp)
add_executable (deliveryoptimization-agent-tests ${files_docs_tests})
target_link_libraries(deliveryoptimization-agent-tests
    docs_common
    dotestutil
    stdc++fs
    cpprestsdk::cpprest
    ${Boost_LIBRARIES}
    GTest::GTest
)
