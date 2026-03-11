cmake_minimum_required(VERSION 3.25)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VCPKG_ROOT} AND EXISTS "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
  elseif(EXISTS "${CMAKE_CURRENT_LIST_DIR}/external/vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/external/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
  endif()
endif()

project(silicore-c VERSION 1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 17)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  add_compile_options(-O3 -march=native -flto -fstack-protector-strong)
endif()

add_compile_options(-Wall -Wextra -Wshadow -Wno-unused-parameter)

if(UNIX AND NOT APPLE)
  add_compile_options(-D_GNU_SOURCE)
endif()

find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

option(SILICORE_USE_LINENOISE "Use linenoise for prompt input" OFF)

include(platforms/platform_sources.cmake)

set(SILICORE_CORE_SOURCES
  src/orchestrator.cpp
  src/runner.cpp
  src/execution_policy.cpp
  src/domain/entity.cpp
  src/engines/async_engine.cpp
  src/engines/cpu_engine.cpp
  src/engines/engine_base.cpp
  src/engines/fusion_engine.cpp
  src/engines/health_monitor.cpp
  src/engines/stabilizer_engine.cpp
  src/engines/scheduler.cpp
  src/engines/thread_engine.cpp
  src/engines/parallel_engine.cpp
  src/collect/platform_schema.cpp
  src/collect/platform_registry.cpp
  src/collect/platform_scanner.cpp
  src/collect/domain_collector.cpp
  ${SILICORE_PLATFORM_SOURCES}
  src/interface/about.cpp
  src/interface/cli_parser.cpp
  src/interface/line_input.cpp
  src/interface/prompt.cpp
  src/interface/banner.cpp
  src/interface/explain.cpp
  src/reporting/reporting.cpp
  src/extensions/plugin_loader.cpp
)

add_library(silicore_core STATIC ${SILICORE_CORE_SOURCES})

target_include_directories(silicore_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})

if(SILICORE_USE_LINENOISE)
  target_compile_definitions(silicore_core PUBLIC SILICORE_USE_LINENOISE)
endif()

target_link_libraries(silicore_core
  PUBLIC
    CURL::libcurl
    nlohmann_json::nlohmann_json
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
)

if(UNIX AND NOT APPLE)
  target_link_libraries(silicore_core PUBLIC dl)
endif()

add_executable(silicore_c src/main.cpp)
set_target_properties(silicore_c PROPERTIES OUTPUT_NAME "silicore-c")

target_link_libraries(silicore_c PRIVATE silicore_core)

set(PLUGIN_OUTPUT_DIR ${CMAKE_BINARY_DIR}/plugins)

add_library(plugin_sample_summary SHARED plugins/sample_summary.cpp)
set_target_properties(plugin_sample_summary PROPERTIES PREFIX "" LIBRARY_OUTPUT_DIRECTORY ${PLUGIN_OUTPUT_DIR})

target_link_libraries(plugin_sample_summary PRIVATE nlohmann_json::nlohmann_json)

enable_testing()
add_subdirectory(tests)
