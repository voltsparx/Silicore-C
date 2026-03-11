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
  src/collect/anonymity.cpp
  src/collect/domain_collector.cpp
  ${SILICORE_PLATFORM_SOURCES}
  src/interface/about.cpp
  src/interface/cli_parser.cpp
  src/interface/line_input.cpp
  src/interface/prompt.cpp
  src/interface/banner.cpp
  src/interface/explain.cpp
  src/interface/live_server.cpp
  src/reporting/reporting.cpp
  src/extensions/plugin_loader.cpp
  src/extensions/filter_loader.cpp
  modules/catalog.cpp
  modules/plugin-modules.cpp
  modules/filter-modules.cpp
  modules/index.cpp
  modules/__init__.cpp
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
if(WIN32)
  target_link_libraries(silicore_core PUBLIC ws2_32)
endif()

add_executable(silicore_c src/main.cpp)
set_target_properties(silicore_c PROPERTIES OUTPUT_NAME "silicore-c")

target_link_libraries(silicore_c PRIVATE silicore_core)

set(PLUGIN_OUTPUT_DIR ${CMAKE_BINARY_DIR}/plugins)

set(SILICORE_PLUGIN_SOURCES
  plugins/account_recovery_exposure_probe.cpp
  plugins/contact_lattice.cpp
  plugins/cross_platform_activity_timeline.cpp
  plugins/domain_takeover_risk_probe.cpp
  plugins/email_pattern_inference.cpp
  plugins/header_hardening_probe.cpp
  plugins/identity_fusion_core.cpp
  plugins/link_outbound_risk_profiler.cpp
  plugins/module_capability_matrix.cpp
  plugins/orbit_link_matrix.cpp
  plugins/rdap_lifecycle_inspector.cpp
  plugins/security_txt_analyzer.cpp
  plugins/signal_fusion_core.cpp
  plugins/subdomain_risk_atlas.cpp
  plugins/surface_transport_stability_probe.cpp
  plugins/threat_conductor.cpp
  plugins/username_impersonation_probe.cpp
  plugins/sample_summary.cpp
  plugins/crypto/aes_plugin.cpp
  plugins/crypto/rot13_plugin.cpp
  plugins/crypto/xor_plugin.cpp
)

foreach(plugin_src IN LISTS SILICORE_PLUGIN_SOURCES)
  get_filename_component(plugin_name ${plugin_src} NAME_WE)
  get_filename_component(plugin_dir ${plugin_src} DIRECTORY)
  string(REPLACE "/" "_" plugin_dir ${plugin_dir})
  string(REPLACE "\\" "_" plugin_dir ${plugin_dir})
  set(plugin_target "plugin_${plugin_dir}_${plugin_name}")
  add_library(${plugin_target} SHARED ${plugin_src})
  set_target_properties(${plugin_target} PROPERTIES PREFIX "" OUTPUT_NAME ${plugin_name} LIBRARY_OUTPUT_DIRECTORY ${PLUGIN_OUTPUT_DIR})
  target_include_directories(${plugin_target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(${plugin_target} PRIVATE nlohmann_json::nlohmann_json)
endforeach()

set(FILTER_OUTPUT_DIR ${CMAKE_BINARY_DIR}/filters)

set(SILICORE_FILTER_SOURCES
  filters/anomaly_detection_filter.cpp
  filters/contact_canonicalizer.cpp
  filters/contact_quality_filter.cpp
  filters/disclosure_readiness_filter.cpp
  filters/entity_name_resolver.cpp
  filters/evidence_consistency_filter.cpp
  filters/exposure_tier_matrix.cpp
  filters/link_hygiene_filter.cpp
  filters/mailbox_provider_profiler.cpp
  filters/module_filter_router.cpp
  filters/noise_suppression_filter.cpp
  filters/pii_signal_classifier.cpp
  filters/sensitive_lexicon_guard.cpp
  filters/signal_lane_fusion.cpp
  filters/subdomain_attack_path_filter.cpp
  filters/takeover_priority_filter.cpp
  filters/triage_priority_filter.cpp
)

foreach(filter_src IN LISTS SILICORE_FILTER_SOURCES)
  get_filename_component(filter_name ${filter_src} NAME_WE)
  get_filename_component(filter_dir ${filter_src} DIRECTORY)
  string(REPLACE "/" "_" filter_dir ${filter_dir})
  string(REPLACE "\\" "_" filter_dir ${filter_dir})
  set(filter_target "filter_${filter_dir}_${filter_name}")
  add_library(${filter_target} SHARED ${filter_src})
  set_target_properties(${filter_target} PROPERTIES PREFIX "" OUTPUT_NAME ${filter_name} LIBRARY_OUTPUT_DIRECTORY ${FILTER_OUTPUT_DIR})
  target_include_directories(${filter_target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(${filter_target} PRIVATE nlohmann_json::nlohmann_json)
endforeach()

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
