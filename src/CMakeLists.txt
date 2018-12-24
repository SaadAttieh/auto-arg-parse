
cmake_minimum_required (VERSION 3.6)

# List of C++ features used.
# Should be `cxx_std_11`, but this style specification was introduced from `CMake` 3.8
set (CXX_FEATURES_USED cxx_auto_type
                       cxx_defaulted_functions
                       cxx_generalized_initializers
                       cxx_variadic_templates)

add_library (autoArgParse INTERFACE)
target_include_directories (autoArgParse INTERFACE
                            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
                            $<INSTALL_INTERFACE:include>)
target_compile_features (autoArgParse INTERFACE ${CXX_FEATURES_USED})
target_compile_definitions (autoArgParse INTERFACE AUTOARGPARSE_HEADER_ONLY=1)
