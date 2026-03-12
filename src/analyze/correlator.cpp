#include "analyze/correlator.h"

#include "utils/strings.h"

#include <algorithm>
#include <map>
#include <numeric>
#include <set>

namespace silicore::analyze {

namespace {

std::map<std::string, std::vector<std::string>> dedupe_bucket(
    const std::map<std::string, std::vector<std::string>>& bucket
) {
    std::map<std::string, std::vector<std::string>> output;
    for (const auto& [key, values] : bucket) {
        std::set<std::string> deduped(values.begin(), values.end());
        if (deduped.size() <= 1) {
            continue;
        }
        std::vector<std::string> sorted_values(deduped.begin(), deduped.end());
        output.emplace(key, std::move(sorted_values));
    }
    return output;
}

std::map<std::string, std::vector<std::string>> bucket_confidence(const std::vector<domain::ProfileEntity>& results) {
    std::map<std::string, std::vector<std::string>> clusters{
        {"high", {}},
        {"medium", {}},
        {"low", {}},
    };
    for (const auto& item : results) {
        int confidence = static_cast<int>(item.confidence * 100.0f);
        const std::string platform = item.platform.empty() ? "unknown" : item.platform;
        if (confidence >= 85) {
            clusters["high"].push_back(platform);
        } else if (confidence >= 60) {
            clusters["medium"].push_back(platform);
        } else if (confidence > 0) {
            clusters["low"].push_back(platform);
        }
    }
    for (auto& [key, values] : clusters) {
        std::sort(values.begin(), values.end());
    }
    return clusters;
}

int average_int(const std::vector<int>& values) {
    if (values.empty()) {
        return 0;
    }
    long total = 0;
    for (auto v : values) {
        total += v;
    }
    return static_cast<int>(total / static_cast<long>(values.size()));
}

} // namespace

CorrelationResult correlate(const std::vector<domain::ProfileEntity>& results) {
    std::map<std::string, std::vector<std::string>> shared_bios;
    std::map<std::string, std::vector<std::string>> shared_emails;
    std::map<std::string, std::vector<std::string>> shared_phones;
    std::map<std::string, std::vector<std::string>> shared_links;
    std::map<std::string, std::vector<std::string>> shared_mentions;

    std::map<std::string, int> status_counter;
    std::vector<int> response_times;
    std::vector<int> confidence_values;

    for (const auto& item : results) {
        status_counter[item.status.empty() ? "UNKNOWN" : item.status] += 1;
        confidence_values.push_back(static_cast<int>(item.confidence * 100.0f));
        if (item.response_time_ms > 0) {
            response_times.push_back(static_cast<int>(item.response_time_ms));
        }

        if (item.status != "FOUND") {
            continue;
        }

        const std::string platform = item.platform.empty() ? "unknown" : item.platform;

        auto bio = utils::trim(item.bio);
        if (!bio.empty()) {
            shared_bios[bio].push_back(platform);
        }

        for (const auto& email : item.contacts.emails) {
            auto normalized = utils::to_lower(utils::trim(email));
            if (!normalized.empty()) {
                shared_emails[normalized].push_back(platform);
            }
        }

        for (const auto& phone : item.contacts.phones) {
            auto normalized = utils::trim(phone);
            if (!normalized.empty()) {
                shared_phones[normalized].push_back(platform);
            }
        }

        for (const auto& link : item.links) {
            auto normalized = utils::trim(link);
            if (!normalized.empty()) {
                shared_links[normalized].push_back(platform);
            }
        }

        for (const auto& mention : item.mentions) {
            auto normalized = utils::to_lower(utils::trim(mention));
            if (!normalized.empty()) {
                shared_mentions[normalized].push_back(platform);
            }
        }
    }

    auto confidence_clusters = bucket_confidence(results);
    int overlap_points = static_cast<int>(dedupe_bucket(shared_bios).size())
        + static_cast<int>(dedupe_bucket(shared_emails).size())
        + static_cast<int>(dedupe_bucket(shared_phones).size())
        + static_cast<int>(dedupe_bucket(shared_links).size());
    int identity_overlap_score = std::min(100, overlap_points * 12 + static_cast<int>(confidence_clusters["high"].size()) * 4);

    ResponseTimeStats stats;
    if (!response_times.empty()) {
        auto [min_it, max_it] = std::minmax_element(response_times.begin(), response_times.end());
        stats.min_ms = *min_it;
        stats.max_ms = *max_it;
        stats.avg_ms = average_int(response_times);
    }

    CorrelationResult result;
    result.shared_bios = dedupe_bucket(shared_bios);
    result.shared_emails = dedupe_bucket(shared_emails);
    result.shared_phones = dedupe_bucket(shared_phones);
    result.shared_links = dedupe_bucket(shared_links);
    result.shared_mentions = dedupe_bucket(shared_mentions);
    result.confidence_cluster_map = confidence_clusters;
    result.confidence_clusters = confidence_clusters["high"];
    result.status_distribution = status_counter;
    result.response_time_stats = stats;
    result.average_confidence = average_int(confidence_values);
    result.identity_overlap_score = identity_overlap_score;
    return result;
}

} // namespace silicore::analyze
