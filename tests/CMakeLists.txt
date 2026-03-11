find_package(GTest REQUIRED)

add_executable(silicore_tests
  test_platform_schema.cpp
  test_profile_classifier.cpp
  test_integration_profile.cpp
  test_domain_utils.cpp
  test_plugin_loader.cpp
)

target_link_libraries(silicore_tests PRIVATE silicore_core GTest::gtest_main)
target_compile_definitions(silicore_tests PRIVATE SILICORE_TEST_PLUGIN_DIR=\"${PLUGIN_OUTPUT_DIR}\")
add_dependencies(silicore_tests plugin_plugins_sample_summary)

include(GoogleTest)
gtest_discover_tests(silicore_tests)
