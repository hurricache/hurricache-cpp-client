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

target_include_directories(client_smoke_test PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)


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

# Queue operations test runner
add_executable(queue_ops_test
    src/test_runner.cxx
    src/list_ops_test.cxx
    src/test_queue_operations.cxx
)
target_include_directories(queue_ops_test PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(queue_ops_test PRIVATE hurricache_standalone_client hurricache_common gRPC::grpc++ protobuf::libprotobuf)

# Vector operations test runner
add_executable(vector_ops_test
    src/test_runner.cxx
    src/list_ops_test.cxx
    src/test_queue_operations.cxx
    src/test_vector_operations.cxx
    src/test_set_operations.cxx
)
target_include_directories(vector_ops_test PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(vector_ops_test PRIVATE hurricache_standalone_client hurricache_common gRPC::grpc++ protobuf::libprotobuf)

# Universal test runner - all tests in one executable
# Note: basic_tests.cxx has its own main(), so it's excluded
add_executable(universal_test_runner
    src/universal_test_runner.cxx
    src/test_ttl.cxx
    src/test_kv_ops.cxx
    src/test_container_creation.cxx
    src/test_container_info.cxx
    src/test_streaming.cxx
    src/test_insertion_deletion.cxx
    src/test_locking_atomic.cxx
    src/list_ops_test.cxx
    src/test_queue_operations.cxx
    src/test_vector_operations.cxx
    src/test_set_operations.cxx
)
target_include_directories(universal_test_runner PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(universal_test_runner PRIVATE hurricache_standalone_client hurricache_common gRPC::grpc++ protobuf::libprotobuf)

# Comprehensive test runner - all tests in one executable
add_executable(comprehensive_test
    src/comprehensive_test_runner.cxx
    src/list_ops_test.cxx
    src/test_queue_operations.cxx
    src/test_vector_operations.cxx
    src/test_set_operations.cxx
)
target_include_directories(comprehensive_test PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(comprehensive_test PRIVATE hurricache_standalone_client hurricache_common gRPC::grpc++ protobuf::libprotobuf)


add_executable(raw_stress_test
        src/raw_stress.cxx
)
target_include_directories(raw_stress_test PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(raw_stress_test PRIVATE hurricache_standalone_client hurricache_common gRPC::grpc++ protobuf::libprotobuf)


