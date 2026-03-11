#include "collect/platform_schema.h"

#include <gtest/gtest.h>
#include <algorithm>

using namespace silicore::collect;

TEST(PlatformSchema, EmbeddedPlatformsPresent) {
    auto platforms = load_platforms("platforms");
    ASSERT_GT(platforms.size(), 0u);

    auto it = std::find_if(platforms.begin(), platforms.end(), [](const PlatformConfig& cfg) {
        return cfg.name == "GitHub";
    });
    EXPECT_TRUE(it != platforms.end());
}

