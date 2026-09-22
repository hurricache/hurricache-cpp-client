cmake_minimum_required(VERSION 3.20)
project(hurricache_common CXX)
include(../grpc-setup.cmake)
set(PROTO_FILE "${CMAKE_SOURCE_DIR}/proto/cache.proto")
set(PROTO_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${PROTO_GEN_DIR}")
find_package(PkgConfig REQUIRED)
find_package(ZLIB REQUIRED)
pkg_check_modules(LIBDEFLATE REQUIRED IMPORTED_TARGET libdeflate)

set(PROTO_SRCS
        "${PROTO_GEN_DIR}/cache.pb.cc"
        "${PROTO_GEN_DIR}/cache.pb.h"
        "${PROTO_GEN_DIR}/cache_grpc.pb.cc"
        "${PROTO_GEN_DIR}/cache_grpc.pb.h"
)

add_custom_command(
        OUTPUT ${PROTO_SRCS}
        COMMAND protobuf::protoc
        ARGS
        --cpp_out="${PROTO_GEN_DIR}"
        --grpc_out="${PROTO_GEN_DIR}"
        --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
        -I "${CMAKE_SOURCE_DIR}/proto"
        "${PROTO_FILE}"
        DEPENDS "${PROTO_FILE}" protobuf::protoc gRPC::grpc_cpp_plugin
        COMMENT "Running gRPC C++ compiler on cache.proto"
)

add_library(hurricache_common STATIC
        ${PROTO_SRCS}
        src/types.cxx
        src/hashes.cxx
        src/utils.cxx
        src/compression.cxx
)

target_compile_features(hurricache_common PUBLIC cxx_std_23)
target_include_directories(hurricache_common PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${PROTO_GEN_DIR}
)

target_link_libraries(hurricache_common PUBLIC
        gRPC::grpc++
        protobuf::libprotobuf
        ZLIB::ZLIB
        PkgConfig::LIBDEFLATE
)