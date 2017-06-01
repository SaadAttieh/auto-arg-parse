
cmake_minimum_required (VERSION 3.8)

set (TARGET_NAME exampleUsage)
add_executable (${TARGET_NAME} exampleUsage.cpp ${AutoArgParse_SOURCE_DIR}/include/autoArgParse/argParser.cpp)
target_include_directories (${TARGET_NAME} PRIVATE ${AutoArgParse_SOURCE_DIR}/include)
target_compile_features (${TARGET_NAME} PRIVATE cxx_std_14)
