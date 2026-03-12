#include "analyze/profile_summary.h"

#include "utils/strings.h"

#include <algorithm>
#include <cctype>
#include <regex>
#include <set>
#include <unordered_map>

namespace silicore::analyze {

namespace {

constexpr const char* kFocusStatuses[] = {"FOUND", "ERROR", "BLOCKED"};
constexpr const char* kErrorStatuses[] = {"ERROR", "BLOCKED"};

bool is_focus_status(const std::string& status) {
    for (const auto* item : kFocusStatuses) {
        if (status == item) {
            return true;
        }
    }
    return false;
}

bool is_error_status(const std::string& status) {
    for (const auto* item : kErrorStatuses) {
        if (status == item) {
            return true;
        }
    }
    return false;
}

std::string extract_domain(const std::string& value) {
    std::string raw = utils::trim(value);
    if (raw.empty()) {
        return "";
    }
    auto scheme_pos = raw.find("://");
    if (scheme_pos == std::string::npos) {
        return "";
    }
    auto host_start = scheme_pos + 3;
    auto host_end = raw.find('/', host_start);
    std::string host = raw.substr(host_start, host_end == std::string::npos ? std::string::npos : host_end - host_start);
    if (utils::starts_with(host, "www.")) {
        host = host.substr(4);
    }
    auto port_pos = host.find(':');
    if (port_pos != std::string::npos) {
        host = host.substr(0, port_pos);
    }
    return utils::to_lower(host);
}

std::vector<std::string> extract_name_candidates(const std::string& text) {
    static const std::regex pattern(R"(\b([A-Z][a-z]{1,24}(?:\s+[A-Z][a-z]{1,24}){1,2})\b)");
    std::vector<std::string> tokens;
    try {
        for (std::sregex_iterator it(text.begin(), text.end(), pattern), end; it != end; ++it) {
            auto token = utils::trim(it->str());
            if (!token.empty()) {
                tokens.push_back(token);
            }
        }
    } catch (const std::regex_error&) {
        return {};
    }
    std::vector<std::string> deduped;
    std::set<std::string> seen;
    for (const auto& item : tokens) {
        auto lowered = utils::to_lower(item);
        if (seen.count(lowered)) {
            continue;
        }
        seen.insert(lowered);
        deduped.push_back(item);
        if (deduped.size() >= 8) {
            break;
        }
    }
    return deduped;
}

template <typename T>
std::vector<T> sorted_unique(std::vector<T> values) {
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
    return values;
}

} // namespace

std::vector<domain::ProfileEntity> focused_profile_rows(const std::vector<domain::ProfileEntity>& results) {
    std::vector<domain::ProfileEntity> rows;
    rows.reserve(results.size());
    for (const auto& row : results) {
        if (is_focus_status(row.status)) {
            rows.push_back(row);
        }
    }
    return rows;
}

std::vector<domain::ProfileEntity> found_profile_rows(const std::vector<domain::ProfileEntity>& results) {
    std::vector<domain::ProfileEntity> rows;
    for (const auto& row : results) {
        if (row.status == "FOUND") {
            rows.push_back(row);
        }
    }
    std::sort(rows.begin(), rows.end(), [](const domain::ProfileEntity& a, const domain::ProfileEntity& b) {
        int a_conf = static_cast<int>(a.confidence * 100.0f);
        int b_conf = static_cast<int>(b.confidence * 100.0f);
        if (a_conf != b_conf) {
            return a_conf > b_conf;
        }
        return utils::to_lower(a.platform) < utils::to_lower(b.platform);
    });
    return rows;
}

std::vector<domain::ProfileEntity> error_profile_rows(const std::vector<domain::ProfileEntity>& results) {
    std::vector<domain::ProfileEntity> rows;
    for (const auto& row : results) {
        if (is_error_status(row.status)) {
            rows.push_back(row);
        }
    }
    std::sort(rows.begin(), rows.end(), [](const domain::ProfileEntity& a, const domain::ProfileEntity& b) {
        auto a_platform = utils::to_lower(a.platform);
        auto b_platform = utils::to_lower(b.platform);
        if (a_platform != b_platform) {
            return a_platform < b_platform;
        }
        return utils::to_lower(a.status) < utils::to_lower(b.status);
    });
    return rows;
}

TargetSnapshot summarize_target_intel(const std::vector<domain::ProfileEntity>& results) {
    auto found_rows = found_profile_rows(results);
    auto error_rows = error_profile_rows(results);

    std::map<std::string, int> status_breakdown;
    std::unordered_map<std::string, int> email_domains;
    std::unordered_map<std::string, int> link_domains;

    std::vector<std::string> found_platforms;
    std::vector<std::string> profile_links;
    std::vector<std::string> emails;
    std::vector<std::string> phones;
    std::vector<std::string> names;
    std::vector<std::string> mentions;
    std::vector<std::string> external_links;
    std::vector<std::string> bios;

    std::vector<long> found_response_times;
    std::vector<long> error_response_times;
    std::vector<int> found_confidences;

    for (const auto& row : results) {
        status_breakdown[row.status]++;
    }

    for (const auto& row : found_rows) {
        if (!row.platform.empty()) {
            found_platforms.push_back(row.platform);
        }
        if (!row.profile_url.empty()) {
            profile_links.push_back(row.profile_url);
        }

        for (const auto& email : row.contacts.emails) {
            auto value = utils::trim(email);
            if (value.empty()) {
                continue;
            }
            emails.push_back(value);
            auto pos = value.find('@');
            if (pos != std::string::npos && pos + 1 < value.size()) {
                auto domain = utils::to_lower(utils::trim(value.substr(pos + 1)));
                if (!domain.empty()) {
                    email_domains[domain]++;
                }
            }
        }

        for (const auto& phone : row.contacts.phones) {
            auto value = utils::trim(phone);
            if (!value.empty()) {
                phones.push_back(value);
            }
        }

        for (const auto& mention : row.mentions) {
            auto value = utils::trim(mention);
            if (!value.empty()) {
                mentions.push_back(value);
            }
        }

        for (const auto& link : row.links) {
            auto value = utils::trim(link);
            if (!value.empty()) {
                external_links.push_back(value);
                auto domain = extract_domain(value);
                if (!domain.empty()) {
                    link_domains[domain]++;
                }
            }
        }

        auto bio = utils::trim(row.bio);
        if (!bio.empty()) {
            bios.push_back(bio);
            auto candidates = extract_name_candidates(bio);
            names.insert(names.end(), candidates.begin(), candidates.end());
        }

        if (row.response_time_ms > 0) {
            found_response_times.push_back(row.response_time_ms);
        }
        int confidence = static_cast<int>(row.confidence * 100.0f);
        found_confidences.push_back(confidence);
    }

    std::vector<ErrorDetail> error_details;
    error_details.reserve(error_rows.size());
    for (const auto& row : error_rows) {
        if (row.response_time_ms > 0) {
            error_response_times.push_back(row.response_time_ms);
        }
        ErrorDetail detail;
        detail.platform = row.platform.empty() ? "Unknown" : row.platform;
        detail.status = row.status.empty() ? "ERROR" : row.status;
        detail.url = row.profile_url;
        detail.http_status = row.http_status;
        detail.response_time_ms = row.response_time_ms;
        detail.context = row.context;
        error_details.push_back(std::move(detail));
    }

    auto total_results = static_cast<int>(results.size());
    auto found_count = static_cast<int>(found_rows.size());
    auto error_count = static_cast<int>(error_rows.size());
    double coverage_ratio = total_results ? static_cast<double>(found_count) / static_cast<double>(total_results) : 0.0;

    auto avg_int = [](const std::vector<long>& values) -> double {
        if (values.empty()) {
            return 0.0;
        }
        long sum = 0;
        for (auto v : values) {
            sum += v;
        }
        return static_cast<double>(sum) / static_cast<double>(values.size());
    };
    auto avg_conf = [&]() -> double {
        if (found_confidences.empty()) {
            return 0.0;
        }
        long sum = 0;
        for (auto v : found_confidences) {
            sum += v;
        }
        return static_cast<double>(sum) / static_cast<double>(found_confidences.size());
    };

    std::vector<std::string> email_domain_tokens;
    email_domain_tokens.reserve(email_domains.size());
    std::vector<std::pair<std::string, int>> email_domain_sorted(email_domains.begin(), email_domains.end());
    std::sort(email_domain_sorted.begin(), email_domain_sorted.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });
    for (size_t i = 0; i < email_domain_sorted.size() && i < 10; ++i) {
        email_domain_tokens.push_back(email_domain_sorted[i].first + ":" + std::to_string(email_domain_sorted[i].second));
    }

    std::vector<std::string> link_domain_tokens;
    link_domain_tokens.reserve(link_domains.size());
    std::vector<std::pair<std::string, int>> link_domain_sorted(link_domains.begin(), link_domains.end());
    std::sort(link_domain_sorted.begin(), link_domain_sorted.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });
    for (size_t i = 0; i < link_domain_sorted.size() && i < 12; ++i) {
        link_domain_tokens.push_back(link_domain_sorted[i].first + ":" + std::to_string(link_domain_sorted[i].second));
    }

    TargetSnapshot snapshot;
    snapshot.total_results = total_results;
    snapshot.found_count = found_count;
    snapshot.error_count = error_count;
    snapshot.coverage_ratio = coverage_ratio;
    snapshot.avg_found_confidence = avg_conf();
    snapshot.avg_found_response_time_ms = avg_int(found_response_times);
    snapshot.avg_error_response_time_ms = avg_int(error_response_times);
    snapshot.status_breakdown = status_breakdown;
    snapshot.found_platforms = sorted_unique(found_platforms);
    snapshot.profile_links = sorted_unique(profile_links);
    snapshot.emails = sorted_unique(emails);
    snapshot.email_domains = email_domain_tokens;
    snapshot.phones = sorted_unique(phones);
    snapshot.names = sorted_unique(names);
    snapshot.mentions = sorted_unique(mentions);
    snapshot.external_links = sorted_unique(external_links);
    snapshot.external_link_domains = link_domain_tokens;
    snapshot.bios = sorted_unique(bios);
    snapshot.errors = std::move(error_details);
    return snapshot;
}

} // namespace silicore::analyze
