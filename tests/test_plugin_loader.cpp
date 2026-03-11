#include "extensions/plugin_loader.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <filesystem>

using namespace silicore::extensions;
using json = nlohmann::json;

#ifndef SILICORE_TEST_PLUGIN_DIR
#define SILICORE_TEST_PLUGIN_DIR "./plugins"
#endif

TEST(PluginLoader, LoadAndRun) {
    std::filesystem::path plugin_dir = SILICORE_TEST_PLUGIN_DIR;
    PluginManager manager;
    manager.load_all(plugin_dir);
    ASSERT_GE(manager.plugins().size(), 1u);

    json context;
    context["results"] = json::array({ { {"status", "FOUND"} } });

    auto results = manager.run_scope("profile", context.dump());
    ASSERT_FALSE(results.empty());

    bool found = false;
    for (const auto& res : results) {
        if (res.id == "sample_summary") {
            found = true;
            json output = json::parse(res.output_json);
            EXPECT_EQ(output.value("found_profiles", 0), 1);
        }
    }
    EXPECT_TRUE(found);
}

