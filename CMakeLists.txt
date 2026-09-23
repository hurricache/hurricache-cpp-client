cmake_minimum_required(VERSION 3.20)
project("hurricache-cpp-client" CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_definitions(
        ABSL_MIN_LOG_LEVEL=4
        GRPC_NO_XDS
        GRPC_ARES=0
        NDEBUG
        GRPC_MINIMAL_LB_POLICY
        GRPC_NO_RLS
)
add_compile_options(
        -march=native
)

# Порядок имеет значение: сначала common, так как он нужен остальным
add_subdirectory(common)
add_subdirectory(smart)
add_subdirectory(standalone)
add_subdirectory(tests)