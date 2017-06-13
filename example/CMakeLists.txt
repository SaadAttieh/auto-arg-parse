
cmake_minimum_required (VERSION 3.6)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wextra -Wall")
add_executable (exampleUsage exampleUsage.cpp)
target_link_libraries (exampleUsage PRIVATE autoArgParse)

add_executable (exampleUsageHeaderOnly exampleUsage.cpp)
target_link_libraries (exampleUsageHeaderOnly PRIVATE autoArgParse-header-only)
