if(TARGET gRPC::grpc++ OR TARGET grpc++)
    return()
endif()

# -----------------------------------------------------------------------------
# 1. Глобальные опции сборки и LTO
# -----------------------------------------------------------------------------
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Force static libraries" FORCE)
set(protobuf_BUILD_SHARED_LIBS OFF CACHE BOOL "Force static protobuf" FORCE)
set(CMAKE_POSITION_INDEPENDENT_CODE ON CACHE BOOL "Force PIC" FORCE)

# Включаем Interprocedural Optimization (LTO) для всех подпроектов
#set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON CACHE BOOL "Enable LTO globally" FORCE)
#set(gRPC_ENABLE_INTERPROCEDURAL_OPTIMIZATION ON CACHE BOOL "Enable LTO for gRPC" FORCE)

# Форсируем флаговые оптимизации для Release-сборки gRPC / Abseil / Protobuf
#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native -mno-avx512f -flto=auto -fno-semantic-interposition" CACHE STRING "" FORCE)
#set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -march=native -mno-avx512f -flto=auto -fno-semantic-interposition" CACHE STRING "" FORCE)

# КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ:
# Включаем инсталляцию/экспорт для Abseil, чтобы его таргеты были доступны для export-сета Protobuf.
# Благодаря EXCLUDE_FROM_ALL эти файлы НЕ будут физически устанавливаться в систему.
set(ABSL_ENABLE_INSTALL ON CACHE BOOL "" FORCE)

# Остальные библиотеки отключаем от инсталляции
set(gRPC_INSTALL OFF CACHE BOOL "" FORCE)
set(protobuf_INSTALL OFF CACHE BOOL "" FORCE)
set(UTF8_RANGE_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(RE2_BUILD_TESTING OFF CACHE BOOL "" FORCE)

# -----------------------------------------------------------------------------
# 2. Stripping компонентов gRPC и Protobuf
# -----------------------------------------------------------------------------
set(protobuf_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_LIBPROTOC ON CACHE BOOL "" FORCE)
set(protobuf_BUILD_PROTOC_BINARIES ON CACHE BOOL "" FORCE)
set(protobuf_DISABLE_RTTI OFF CACHE BOOL "" FORCE)

set(gRPC_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(gRPC_BUILD_CSHARP_EXT OFF CACHE BOOL "" FORCE)
set(gRPC_BUILD_PYTHON_EXT OFF CACHE BOOL "" FORCE)
set(gRPC_BUILD_GRPC_CSHARP_PLUGIN OFF CACHE BOOL "" FORCE)
set(gRPC_BUILD_GRPC_NODE_PLUGIN OFF CACHE BOOL "" FORCE)
set(gRPC_BUILD_GRPC_OBJECTIVE_C_PLUGIN OFF CACHE BOOL "" FORCE)
set(gRPC_BUILD_GRPC_PHP_PLUGIN OFF CACHE BOOL "" FORCE)
set(gRPC_BUILD_GRPC_PYTHON_PLUGIN OFF CACHE BOOL "" FORCE)
set(gRPC_BUILD_GRPC_RUBY_PLUGIN OFF CACHE BOOL "" FORCE)
set(gRPC_BUILD_GRPC_CPP_PLUGIN ON CACHE BOOL "" FORCE)

set(gRPC_BUILD_GRPCPP_OTEL_PLUGIN OFF CACHE BOOL "" FORCE)
set(gRPC_BUILD_CSDS_STATUS_SYNC_PLUGIN OFF CACHE BOOL "" FORCE)
set(gRPC_BUILD_XDS_HEALTH_CHECK_PROVIDER OFF CACHE BOOL "" FORCE)
set(gRPC_BUILD_GRPCPP_OPENCENSUS_PLUGIN OFF CACHE BOOL "" FORCE)
set(gRPC_BUILD_XDS_CONFIG_CUSTOM_ORGANIZATION OFF CACHE BOOL "" FORCE)
set(gRPC_BUILD_XDS_SECURE_ENDPOINT OFF CACHE BOOL "" FORCE)
set(gRPC_BUILD_GRPCPP_OTEL OFF CACHE BOOL "" FORCE)
set(gRPC_BUILD_XDS_CONFIG OFF CACHE BOOL "" FORCE)
set(gRPC_ENABLE_STATS OFF CACHE BOOL "" FORCE)
set(gRPC_USE_SYSTEMD OFF CACHE BOOL "" FORCE)

# Провайдеры зависимостей
set(gRPC_ABSL_PROVIDER "module" CACHE STRING "" FORCE)
set(gRPC_PROTOBUF_PROVIDER "module" CACHE STRING "" FORCE)
set(gRPC_RE2_PROVIDER "module" CACHE STRING "" FORCE)
set(gRPC_SSL_PROVIDER "module" CACHE STRING "" FORCE)
set(gRPC_ZLIB_PROVIDER "module" CACHE STRING "" FORCE)
set(gRPC_CARES_PROVIDER "none" CACHE STRING "" FORCE)
set(gRPC_BENCHMARK_PROVIDER "none" CACHE STRING "" FORCE)
set(gRPC_USE_SYSTEM_ATOMICS ON CACHE BOOL "" FORCE)
# Отключаем тесты и бенчмарки Abseil
set(ABSL_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(ABSL_ENABLE_EXCEPTIONS OFF CACHE BOOL "" FORCE)

add_compile_definitions(
        DISABLE_STATS_GRPC_STATS
        gRPC_DEBUG_LOGGING=OFF
        ABSL_MIN_LOG_LEVEL=4
        GRPC_NO_XDS
        GRPC_ARES=0
        NDEBUG
        GRPC_MINIMAL_LB_POLICY
        GRPC_NO_RLS
        GPR_LOW_LEVEL_COUNTERS
        GPR_FORBID_UNALIGNED_ACCESS=0
        ABSL_FORCE_THREAD_IDENTITY_MODE=ABSL_THREAD_IDENTITY_MODE_USE_TLS
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(
            GRPC_ASAN_SUPPRESSED=0
    )
endif()

# -----------------------------------------------------------------------------
# 3. Подключение сабмодуля gRPC
# -----------------------------------------------------------------------------
set(GRPC_SUBMODULE_DIR "${CMAKE_SOURCE_DIR}/third_party/grpc")

if(NOT EXISTS "${GRPC_SUBMODULE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "gRPC submodule not found in ${GRPC_SUBMODULE_DIR}!")
endif()

add_subdirectory("${GRPC_SUBMODULE_DIR}" "${CMAKE_BINARY_DIR}/third_party/grpc" EXCLUDE_FROM_ALL)

# -----------------------------------------------------------------------------
# 4. Видимость символов и форсирование LTO для таргетов
# -----------------------------------------------------------------------------
set(GRPC_TARGETS_TO_OPTIMIZE
        grpc
        grpc++
        grpc_unsecure
        grpc++_unsecure
        upb
        address_sorting
)

foreach(target IN LISTS GRPC_TARGETS_TO_OPTIMIZE)
    if(TARGET ${target})
        set_target_properties(${target} PROPERTIES
                CXX_VISIBILITY_PRESET hidden
                VISIBILITY_INLINES_HIDDEN ON
#                INTERPROCEDURAL_OPTIMIZATION ON
        )
        target_compile_definitions(${target} PRIVATE GRPC_NO_ABSL_FLAGS)
    endif()
endforeach()

# -----------------------------------------------------------------------------
# 5. Aliases
# -----------------------------------------------------------------------------
if(TARGET grpc++ AND NOT TARGET gRPC::grpc++)
    add_library(gRPC::grpc++ ALIAS grpc++)
endif()

if(TARGET libprotobuf AND NOT TARGET protobuf::libprotobuf)
    add_library(protobuf::libprotobuf ALIAS libprotobuf)
endif()

if(TARGET protoc AND NOT TARGET protobuf::protoc)
    add_executable(protobuf::protoc ALIAS protoc)
endif()

if(TARGET grpc_cpp_plugin AND NOT TARGET gRPC::grpc_cpp_plugin)
    add_executable(gRPC::grpc_cpp_plugin ALIAS grpc_cpp_plugin)
endif()