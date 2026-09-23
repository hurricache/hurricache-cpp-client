cmake_minimum_required(VERSION 3.20)
project(hurricache_smart CXX)
include(../grpc-setup.cmake)

# Путь к proto-файлу координатора
set(COORD_PROTO_FILE "${CMAKE_SOURCE_DIR}/proto/coordinator.proto")
set(COORD_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated_coord")
file(MAKE_DIRECTORY "${COORD_GEN_DIR}")

set(COORD_PROTO_SRCS
        "${COORD_GEN_DIR}/coordinator.pb.cc"
        "${COORD_GEN_DIR}/coordinator.pb.h"
        "${COORD_GEN_DIR}/coordinator_grpc.pb.cc"
        "${COORD_GEN_DIR}/coordinator_grpc.pb.h"
)

# Генерация файлов из coordinator.proto
add_custom_command(
        OUTPUT ${COORD_PROTO_SRCS}
        COMMAND protobuf::protoc
        ARGS
        --cpp_out="${COORD_GEN_DIR}"
        --grpc_out="${COORD_GEN_DIR}"
        --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
        -I "${CMAKE_SOURCE_DIR}/proto"
        "${COORD_PROTO_FILE}"
        DEPENDS "${COORD_PROTO_FILE}" protobuf::protoc gRPC::grpc_cpp_plugin
        COMMENT "Running gRPC C++ compiler on coordinator.proto"
)

# Собираем смарт-клиент
add_library(hurricache_smart_client STATIC
        src/smart_client.cpp
        # Другие исходники смарт-клиента (например, логика роутинга через координатор)
        ${COORD_PROTO_SRCS}
)

target_compile_features(hurricache_smart_client PUBLIC cxx_std_23)

target_include_directories(hurricache_smart_client PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        PUBLIC
        "${COORD_GEN_DIR}"
)

# Смарт-клиент использует и общий хурикэш (cache.proto), и свой координатор
target_link_libraries(hurricache_smart_client PUBLIC
        hurricache_common
        gRPC::grpc++
        protobuf::libprotobuf
)

set_target_properties(hurricache_smart_client PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
)