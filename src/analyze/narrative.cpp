#include "analyze/narrative.h"

#include "utils/strings.h"

#include <sstream>

namespace silicore::analyze {

namespace {

int safe_int(int value, int fallback = 0) {
    return value;
}

std::string severity_headline(const std::unordered_map<std::string, int>& breakdown) {
    if (breakdown.empty()) {
        return "No explicit risk indicators were triggered";
    }
    auto critical = breakdown.find("CRITICAL");
    if (critical != breakdown.end() && critical->second > 0) {
        return "Critical findings were identified";
    }
    auto high = breakdown.find("HIGH");
    if (high != breakdown.end() && high->second > 0) {
        return "High-severity findings were identified";
    }
    auto medium = breakdown.find("MEDIUM");
    if (medium != breakdown.end() && medium->second > 0) {
        return "Moderate risk findings were identified";
    }
    return "Only low-severity findings were identified";
}

std::string plural(int value, const std::string& singular, const std::string& plural_form = "") {
    if (value == 1) {
        return singular;
    }
    if (!plural_form.empty()) {
        return plural_form;
    }
    return singular + "s";
}

} // namespace

std::string build_nano_brief(
    const std::string& username,
    const std::vector<domain::ProfileEntity>& profile_results,
    const std::string& domain,
    const collect::DomainScanResult* domain_result,
    const std::vector<Issue>&,
    const IssueSummary& issue_summary,
    const CorrelationResult& correlation
) {
    int found_profiles = 0;
    int blocked_profiles = 0;
    for (const auto& item : profile_results) {
        if (item.status == "FOUND") {
            found_profiles++;
        } else if (item.status == "BLOCKED") {
            blocked_profiles++;
        }
    }
    int total_profiles = static_cast<int>(profile_results.size());

    int shared_emails = static_cast<int>(correlation.shared_emails.size());
    int shared_phones = static_cast<int>(correlation.shared_phones.size());
    int shared_bios = static_cast<int>(correlation.shared_bios.size());

    int risk_score = safe_int(issue_summary.risk_score, 0);
    int total_issues = safe_int(issue_summary.total, 0);

    std::string subject = !username.empty() ? username : (!domain.empty() ? domain : "target");
    std::vector<std::string> lines;

    if (!profile_results.empty()) {
        std::ostringstream oss;
        oss << "For " << subject << ", " << found_profiles << " of " << total_profiles
            << " profile checks resolved as FOUND with " << blocked_profiles << " blocked responses.";
        lines.push_back(oss.str());
    }

    if (domain_result) {
        int subdomain_count = static_cast<int>(domain_result->subdomains.size());
        int address_count = static_cast<int>(domain_result->resolved_addresses.size());
        std::ostringstream oss;
        oss << "Domain surface telemetry captured " << subdomain_count << " subdomain candidate "
            << plural(subdomain_count, "entry") << " and " << address_count << " resolved "
            << plural(address_count, "address") << ".";
        lines.push_back(oss.str());
    }

    if (shared_bios || shared_emails || shared_phones) {
        std::ostringstream oss;
        oss << "Correlation identified " << shared_bios << " shared bios, " << shared_emails
            << " shared emails, and " << shared_phones << " shared phones across discovered assets.";
        lines.push_back(oss.str());
    }

    if (total_issues) {
        std::ostringstream oss;
        oss << severity_headline(issue_summary.severity_breakdown) << " across " << total_issues << " "
            << plural(total_issues, "issue") << "; aggregate risk score is " << risk_score << ".";
        lines.push_back(oss.str());
    } else {
        lines.push_back("No material exposure findings were produced by the current heuristic pass.");
    }

    lines.push_back(
        "This narrative is auto-generated per run and updates as new scan artifacts alter confidence, exposure, and correlation signals."
    );

    std::ostringstream out;
    for (size_t i = 0; i < lines.size(); ++i) {
        if (i > 0) {
            out << " ";
        }
        out << lines[i];
    }
    return out.str();
}

} // namespace silicore::analyze
