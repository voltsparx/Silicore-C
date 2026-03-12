#include "collect/platform_scanner.h"

#include "collect/extractor.h"
#include "utils/strings.h"

#include <regex>
#include <algorithm>
#include <chrono>

namespace silicore::collect {

PlatformScanner::PlatformScanner(std::vector<PlatformConfig> configs) : configs_(std::move(configs)) {}

namespace {

bool contains_int(const std::vector<int>& values, int needle) {
    return std::find(values.begin(), values.end(), needle) != values.end();
}

bool matches_regex(const std::string& body, const std::string& pattern) {
    if (pattern.empty()) {
        return true;
    }
    try {
        std::regex re(pattern, std::regex::icase | std::regex::optimize);
        return std::regex_search(body, re);
    } catch (const std::regex_error&) {
        return false;
    }
}

std::string normalize_url(std::string url) {
    url = utils::trim(url);
    if (!url.empty() && url.back() == '/') {
        url.pop_back();
    }
    return utils::to_lower(url);
}

bool has_method(const std::vector<std::string>& methods, const std::string& name) {
    return std::find(methods.begin(), methods.end(), name) != methods.end();
}

} // namespace

std::string classify_profile_status(const PlatformConfig& cfg, const engines::HttpResponse& response, const std::string& username) {
    if (response.status_code < 0 || !response.error.empty()) {
        return "ERROR";
    }

    if (response.status_code == 429 || response.status_code == 403) {
        return "BLOCKED";
    }

    bool is_not_found = false;

    if (has_method(cfg.detection_methods, "status_code") && contains_int(cfg.not_found_statuses, response.status_code)) {
        is_not_found = true;
    }

    if (has_method(cfg.detection_methods, "message") && !cfg.error_messages.empty()) {
        for (const auto& msg : cfg.error_messages) {
            if (utils::contains_case_insensitive(response.body, msg)) {
                is_not_found = true;
                break;
            }
        }
    }

    if (has_method(cfg.detection_methods, "response_url") && !cfg.error_url.empty() && !response.final_url.empty()) {
        std::string expected = utils::replace_copy(cfg.error_url, "{username}", username);
        if (normalize_url(expected) == normalize_url(response.final_url)) {
            is_not_found = true;
        }
    }

    if (!cfg.regex_check.empty() && !matches_regex(response.body, cfg.regex_check)) {
        is_not_found = true;
    }

    if (is_not_found) {
        return "NOT FOUND";
    }

    if (has_method(cfg.detection_methods, "status_code") && !cfg.exists_statuses.empty() && contains_int(cfg.exists_statuses, response.status_code)) {
        return "FOUND";
    }

    if (response.status_code >= 200 && response.status_code < 400) {
        return "FOUND";
    }

    return "ERROR";
}

ProfileScanResult PlatformScanner::scan(
    const std::string& username,
    int concurrency_limit,
    int timeout_ms,
    const std::string& proxy_url
) {
    std::vector<engines::HttpRequest> requests;
    requests.reserve(configs_.size());

    for (const auto& cfg : configs_) {
        engines::HttpRequest req;
        req.url = utils::replace_copy(cfg.url_probe, "{username}", username);
        req.method = cfg.request_method;
        req.timeout_ms = timeout_ms;
        req.proxy_url = proxy_url;
        requests.push_back(req);
    }

    auto responses = engines::run_async_batch(requests, concurrency_limit);

    ProfileScanResult result;
    result.profiles.reserve(configs_.size());

    auto now = std::chrono::system_clock::now();
    for (size_t i = 0; i < configs_.size(); ++i) {
        const auto& cfg = configs_[i];
        const auto& resp = responses[i];

        domain::ProfileEntity entity;
        entity.entity_type = domain::EntityType::Profile;
        entity.platform = cfg.name;
        entity.profile_url = utils::replace_copy(cfg.url, "{username}", username);
        entity.status = classify_profile_status(cfg, resp, username);
        entity.http_status = resp.status_code;
        entity.response_time_ms = resp.elapsed_ms;
        entity.source = "platform";
        entity.value = username;
        entity.confidence = (entity.status == "FOUND") ? static_cast<float>(cfg.confidence_weight) : 0.0f;
        entity.timestamp = now;
        entity.id = domain::make_id("profile", cfg.name, entity.profile_url);
        entity.context = resp.error;
        if (entity.context.empty() && entity.status == "ERROR") {
            entity.context = "http_error";
        }
        if (entity.status == "FOUND") {
            entity.bio = extract_bio(resp.body);
            entity.links = extract_links(resp.body);
            entity.contacts = extract_contacts(resp.body);
            entity.mentions = extract_username_mentions(resp.body, username);
        }
        result.profiles.push_back(std::move(entity));
    }

    return result;
}

} // namespace silicore::collect

