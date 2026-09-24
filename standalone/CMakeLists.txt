cmake_minimum_required(VERSION 3.20)
project(hurricache_standalone CXX)

add_library(hurricache_standalone_client STATIC
        src/standalone_client.cpp  # Replace with your actual source files
        # ... other standalone client sources
)

target_compile_features(hurricache_standalone_client PUBLIC cxx_std_23)

target_include_directories(hurricache_standalone_client PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Standalone client also references common
target_link_libraries(hurricache_standalone_client PUBLIC
        hurricache_common
)

set_target_properties(hurricache_standalone_client PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
)