cmake_minimum_required(VERSION 3.20)
project(hurricache_tests CXX)

# Main smoke test (legacy)
add_executable(client_smoke_test
    src/basic_tests.cxx
    src/test_ttl.cxx
    src/test_kv_ops.cxx
    src/test_container_creation.cxx
    src/test_container_info.cxx
    src/test_streaming.cxx
    src/test_insertion_deletion.cxx
    src/test_locking_atomic.cxx
)

target_link_libraries(client_smoke_test PRIVATE hurricache_standalone_client hurricache_common gRPC::grpc++ protobuf::libprotobuf)


# List operations test runner
add_executable(list_ops_test
    src/test_runner.cxx
    src/list_ops_test.cxx
)
target_include_directories(list_ops_test PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)


target_link_libraries(list_ops_test PRIVATE hurricache_standalone_client hurricache_common gRPC::grpc++ protobuf::libprotobuf)
