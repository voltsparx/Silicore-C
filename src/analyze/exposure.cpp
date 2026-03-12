#include "analyze/exposure.h"

#include "utils/strings.h"

#include <unordered_map>

namespace silicore::analyze {

namespace {

const std::unordered_map<std::string, int> kSeverityWeights = {
    {"LOW", 1},
    {"MEDIUM", 2},
    {"HIGH", 3},
    {"CRITICAL", 5},
};

Issue make_issue(
    std::string scope,
    std::string severity,
    std::string title,
    std::string evidence,
    std::string recommendation
) {
    Issue issue;
    issue.scope = std::move(scope);
    issue.severity = std::move(severity);
    issue.title = std::move(title);
    issue.evidence = std::move(evidence);
    issue.recommendation = std::move(recommendation);
    return issue;
}

} // namespace

std::vector<Issue> assess_profile_exposure(const std::vector<domain::ProfileEntity>& results) {
    std::vector<Issue> issues;
    int found_count = 0;
    int blocked_count = 0;
    int public_emails = 0;
    int public_phones = 0;

    for (const auto& item : results) {
        if (item.status == "FOUND") {
            found_count++;
            public_emails += static_cast<int>(item.contacts.emails.size());
            public_phones += static_cast<int>(item.contacts.phones.size());
        } else if (item.status == "BLOCKED") {
            blocked_count++;
        }
    }

    if (found_count >= 10) {
        issues.push_back(make_issue(
            "identity",
            "MEDIUM",
            "Large Public Account Footprint",
            std::to_string(found_count) + " discoverable profiles were found.",
            "Review account privacy settings and consolidate identity hygiene."
        ));
    }

    if (public_emails > 0) {
        issues.push_back(make_issue(
            "identity",
            "HIGH",
            "Public Email Exposure",
            std::to_string(public_emails) + " email artifact(s) were extracted from public pages.",
            "Rotate exposed addresses or use role-based/public-only aliases."
        ));
    }

    if (public_phones > 0) {
        issues.push_back(make_issue(
            "identity",
            "HIGH",
            "Public Phone Number Exposure",
            std::to_string(public_phones) + " phone artifact(s) were extracted from public pages.",
            "Move sensitive phone numbers behind private channels."
        ));
    }

    if (blocked_count > 0) {
        issues.push_back(make_issue(
            "collection",
            "LOW",
            "Anti-Bot Countermeasures Observed",
            std::to_string(blocked_count) + " platform checks returned blocking fingerprints.",
            "Use validated legal collection pathways and diversify request profiles."
        ));
    }

    return issues;
}

std::vector<Issue> assess_domain_exposure(
    const std::string& domain,
    const std::unordered_map<std::string, std::string>& https_headers,
    bool http_redirects_to_https,
    int certificate_transparency_count
) {
    std::vector<Issue> issues;
    std::unordered_map<std::string, std::string> normalized;
    for (const auto& [key, value] : https_headers) {
        normalized[utils::to_lower(key)] = value;
    }

    if (!http_redirects_to_https) {
        issues.push_back(make_issue(
            domain,
            "MEDIUM",
            "HTTP Not Strictly Redirected",
            "HTTP endpoint did not clearly redirect to HTTPS.",
            "Enforce full HTTP->HTTPS redirects at edge and app layers."
        ));
    }

    if (!normalized.count("strict-transport-security")) {
        issues.push_back(make_issue(
            domain,
            "HIGH",
            "Missing HSTS Header",
            "Strict-Transport-Security header not observed.",
            "Enable HSTS with an adequate max-age and includeSubDomains where appropriate."
        ));
    }

    if (!normalized.count("content-security-policy")) {
        issues.push_back(make_issue(
            domain,
            "MEDIUM",
            "Missing Content-Security-Policy",
            "Content-Security-Policy header not observed.",
            "Deploy a restrictive CSP and iterate in report-only mode first if needed."
        ));
    }

    if (!normalized.count("x-frame-options")) {
        issues.push_back(make_issue(
            domain,
            "MEDIUM",
            "Missing X-Frame-Options",
            "X-Frame-Options header not observed.",
            "Set X-Frame-Options to DENY or SAMEORIGIN."
        ));
    }

    auto server_it = normalized.find("server");
    if (server_it != normalized.end() && !server_it->second.empty()) {
        std::string banner = server_it->second.substr(0, 100);
        issues.push_back(make_issue(
            domain,
            "LOW",
            "Server Banner Disclosure",
            "Server header exposed: " + banner,
            "Reduce banner detail at the reverse proxy/web server layer."
        ));
    }

    auto powered_it = normalized.find("x-powered-by");
    if (powered_it != normalized.end() && !powered_it->second.empty()) {
        std::string banner = powered_it->second.substr(0, 100);
        issues.push_back(make_issue(
            domain,
            "LOW",
            "Technology Banner Disclosure",
            "X-Powered-By exposed: " + banner,
            "Disable X-Powered-By and related framework fingerprinting headers."
        ));
    }

    if (certificate_transparency_count >= 100) {
        issues.push_back(make_issue(
            domain,
            "MEDIUM",
            "High Subdomain Attack Surface",
            std::to_string(certificate_transparency_count) + " CT subdomain observations were collected.",
            "Continuously inventory and decommission stale or shadow subdomains."
        ));
    }

    return issues;
}

IssueSummary summarize_issues(const std::vector<Issue>& issues) {
    IssueSummary summary;
    summary.total = static_cast<int>(issues.size());
    summary.risk_score = 0;
    for (const auto& issue : issues) {
        auto severity = utils::to_upper(issue.severity);
        summary.severity_breakdown[severity]++;
        auto it = kSeverityWeights.find(severity);
        summary.risk_score += (it == kSeverityWeights.end()) ? 1 : it->second;
    }
    return summary;
}

} // namespace silicore::analyze
