# ===================================================================
#                    HFT STRADDLE TRADING SYSTEM
#                         CMAKE BUILD SYSTEM
# ===================================================================
# 
# Professional CMake configuration for high-performance trading system
# Optimized for ultra-low latency and maximum performance
# 
# FEATURES:
# - Compiler optimizations for speed
# - NUMA and SIMD support
# - Static linking for deployment
# - Debug and Release configurations
# - Professional dependency management
# 
# BUILD INSTRUCTIONS:
# mkdir build && cd build
# cmake -DCMAKE_BUILD_TYPE=Release ..
# make -j$(nproc)
# 
# ===================================================================

cmake_minimum_required(VERSION 3.16)
project(HFTStraddleSystem VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler-specific optimizations
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # High-performance compilation flags
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -mtune=native")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -ffast-math -funroll-loops")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto -fno-plt")
    
    # SIMD optimizations
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -mavx2 -mfma")
    
    # Memory optimizations
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fno-stack-protector")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fomit-frame-pointer")
    
    # Cache optimizations
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -falign-functions=32")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -falign-loops=32")
    
    # Warning flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")
    
    # Debug flags
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -DDEBUG -fsanitize=address")
endif()

# Link-time optimizations
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
endif()

# Find required packages
find_package(Threads REQUIRED)

# Optional packages for enhanced features
find_package(PkgConfig)

# NUMA support (optional)
if(PkgConfig_FOUND)
    pkg_check_modules(NUMA numa)
endif()

# Curl for HTTP requests (market data)
find_package(CURL)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

# Source files
set(CORE_SOURCES
    src/main.cpp
    # Add implementation files here when created
    # src/market_data.cpp
    # src/data_ingestion.cpp
    # src/straddle_strategy.cpp
    # src/tech_stock_selector.cpp
)

# Header files for IDE support
set(HEADER_FILES
    include/hft_straddle_system.h
    include/market_data.h
    include/data_ingestion.h
    include/straddle_strategy.h
    include/tech_stock_selector.h
)

# Create main executable
add_executable(hft_straddle ${CORE_SOURCES} ${HEADER_FILES})

# Link libraries
target_link_libraries(hft_straddle 
    PRIVATE 
    Threads::Threads
)

# Optional library linking
if(NUMA_FOUND)
    target_link_libraries(hft_straddle PRIVATE ${NUMA_LIBRARIES})
    target_include_directories(hft_straddle PRIVATE ${NUMA_INCLUDE_DIRS})
    target_compile_definitions(hft_straddle PRIVATE HAS_NUMA=1)
endif()

if(CURL_FOUND)
    target_link_libraries(hft_straddle PRIVATE ${CURL_LIBRARIES})
    target_include_directories(hft_straddle PRIVATE ${CURL_INCLUDE_DIRS})
    target_compile_definitions(hft_straddle PRIVATE HAS_CURL=1)
endif()

# Static linking for deployment
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_link_libraries(hft_straddle PRIVATE -static-libgcc -static-libstdc++)
endif()

# CPU architecture detection and optimization
include(CheckCXXSourceRuns)

# Test for AVX2 support
check_cxx_source_runs("
#include <immintrin.h>
int main() {
    __m256d a = _mm256_set1_pd(1.0);
    __m256d b = _mm256_set1_pd(2.0);
    __m256d c = _mm256_add_pd(a, b);
    return 0;
}
" HAS_AVX2)

if(HAS_AVX2)
    target_compile_definitions(hft_straddle PRIVATE HAS_AVX2=1)
    message(STATUS "AVX2 support detected and enabled")
endif()

# Memory hugepages support (Linux)
if(UNIX AND NOT APPLE)
    target_compile_definitions(hft_straddle PRIVATE HAS_HUGEPAGES=1)
endif()

# Build configuration summary
message(STATUS "")
message(STATUS "HFT Straddle Trading System Build Configuration:")
message(STATUS "================================================")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "System: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_VERSION}")
message(STATUS "Processor: ${CMAKE_SYSTEM_PROCESSOR}")

if(NUMA_FOUND)
    message(STATUS "NUMA Support: Enabled")
else()
    message(STATUS "NUMA Support: Disabled (install libnuma-dev for NUMA support)")
endif()

if(CURL_FOUND)
    message(STATUS "HTTP Support: Enabled")
else()
    message(STATUS "HTTP Support: Disabled (install libcurl-dev for HTTP support)")
endif()

if(HAS_AVX2)
    message(STATUS "SIMD Support: AVX2 Enabled")
else()
    message(STATUS "SIMD Support: Limited (AVX2 not available)")
endif()

message(STATUS "")

# Install configuration
install(TARGETS hft_straddle 
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Install configuration files
install(FILES 
    config/system_config.json
    config/strategy_config.json
    DESTINATION etc/hft_straddle
    OPTIONAL
)

# Create directories for runtime
install(DIRECTORY DESTINATION var/log/hft_straddle)
install(DIRECTORY DESTINATION var/data/hft_straddle)

# CPack configuration for packaging
set(CPACK_PACKAGE_NAME "HFTStraddleSystem")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "High-Performance Options Straddle Trading System")
set(CPACK_PACKAGE_VENDOR "Nadkalpur Manjunath")
set(CPACK_PACKAGE_CONTACT "nsmanju@github.com")

set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libstdc++6, libcurl4")
set(CPACK_RPM_PACKAGE_REQUIRES "glibc, libstdc++, libcurl")

include(CPack)

# Custom targets for development

# Performance profiling target
add_custom_target(profile
    COMMAND valgrind --tool=callgrind --callgrind-out-file=callgrind.out ./hft_straddle
    DEPENDS hft_straddle
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running performance profiler..."
)

# Memory analysis target
add_custom_target(memcheck
    COMMAND valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all ./hft_straddle
    DEPENDS hft_straddle
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running memory analysis..."
)

# Benchmark target (when implemented)
add_custom_target(benchmark
    COMMAND ./hft_straddle --benchmark --no-trading
    DEPENDS hft_straddle
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running system benchmarks..."
)

# Clean additional files
set_directory_properties(PROPERTIES 
    ADDITIONAL_MAKE_CLEAN_FILES "callgrind.out;*.log;*.csv"
)