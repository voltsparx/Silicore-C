#include "core/collect/platform_scanner.h"

#include <gtest/gtest.h>

using namespace silicore::collect;
using silicore::engines::HttpResponse;

TEST(ProfileClassifier, StatusDecisions) {
    PlatformConfig cfg;
    cfg.name = "Example";
    cfg.url = "https://example.com/{username}";
    cfg.url_probe = cfg.url;
    cfg.request_method = "GET";
    cfg.detection_methods = {"status_code", "message"};
    cfg.not_found_statuses = {404};
    cfg.exists_statuses = {200};
    cfg.error_messages = {"not found"};

    HttpResponse resp;
    resp.status_code = 404;
    EXPECT_EQ(classify_profile_status(cfg, resp, "alice"), "NOT_FOUND");

    resp.status_code = 200;
    resp.body = "profile not found";
    EXPECT_EQ(classify_profile_status(cfg, resp, "alice"), "NOT_FOUND");

    resp.status_code = 429;
    resp.body.clear();
    EXPECT_EQ(classify_profile_status(cfg, resp, "alice"), "BLOCKED");

    resp.status_code = 200;
    resp.body = "welcome";
    EXPECT_EQ(classify_profile_status(cfg, resp, "alice"), "FOUND");
}
