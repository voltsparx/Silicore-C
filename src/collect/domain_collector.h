#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace silicore::collect {

struct HttpArtifact {
    int status = -1;
    std::string final_url;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
    std::string error;
    long elapsed_ms = 0;
};

struct RdapInfo {
    std::string handle;
    std::string registrar;
    std::vector<std::string> name_servers;
};

struct DomainScanResult {
    std::string target_domain;
    std::vector<std::string> resolved_addresses;
    HttpArtifact https;
    HttpArtifact http;
    bool http_redirects_to_https = false;
    std::vector<std::string> subdomains;
    RdapInfo rdap;
    bool robots_txt_present = false;
    std::string robots_preview;
    bool security_txt_present = false;
    std::string security_preview;
    std::vector<std::string> scan_notes;
};

struct DomainScanOptions {
    int timeout_ms = 20000;
    bool include_ct = true;
    bool include_rdap = true;
    int max_subdomains = 250;
    int concurrency = 25;
    std::string proxy_url;
    std::string ct_base_url;
    std::string rdap_base_url;
};

std::string normalize_domain(std::string value);

DomainScanResult collect_domain_surface(
    const std::string& domain,
    const DomainScanOptions& options
);

std::vector<std::string> parse_ct_subdomains(const std::string& json_body, const std::string& domain, int max_subdomains);
RdapInfo parse_rdap_info(const std::string& json_body);

} // namespace silicore::collect
