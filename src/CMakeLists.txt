
cmake_minimum_required (VERSION 3.8)

add_library (autoArgParse STATIC ${PROJECT_SOURCE_DIR}/include/autoArgParse/argParser.cpp)
target_compile_features (autoArgParse PUBLIC cxx_std_14)
target_include_directories (autoArgParse PUBLIC
                            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                            $<INSTALL_INTERFACE:include>)

add_library (autoArgParse-header-only INTERFACE)
target_include_directories (autoArgParse-header-only INTERFACE
                            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                            $<INSTALL_INTERFACE:include>)
target_compile_features (autoArgParse-header-only INTERFACE cxx_std_14)
target_compile_definitions (autoArgParse-header-only INTERFACE AUTOARGPARSE_HEADER_ONLY=1)

add_executable (exampleUsage exampleUsage.cpp)
target_link_libraries (exampleUsage PRIVATE autoArgParse)

add_executable (exampleUsageHeaderOnly exampleUsage.cpp)
target_link_libraries (exampleUsageHeaderOnly PRIVATE autoArgParse-header-only)

