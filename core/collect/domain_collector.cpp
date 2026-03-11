#include "core/collect/domain_collector.h"

#include "core/engines/async_engine.h"
#include "core/utils/strings.h"

#include <nlohmann/json.hpp>
#include <algorithm>
#include <chrono>
#include <future>
#include <netdb.h>
#include <set>
#include <sys/socket.h>
#include <arpa/inet.h>

namespace silicore::collect {

using json = nlohmann::json;

namespace {

HttpArtifact to_artifact(const engines::HttpResponse& resp) {
    HttpArtifact out;
    out.status = resp.status_code;
    out.final_url = resp.final_url;
    out.headers = resp.headers;
    out.body = resp.body;
    out.error = resp.error;
    out.elapsed_ms = resp.elapsed_ms;
    return out;
}

std::vector<std::string> resolve_addresses(const std::string& domain) {
    std::vector<std::string> results;
    addrinfo hints{};
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;

    addrinfo* res = nullptr;
    if (getaddrinfo(domain.c_str(), nullptr, &hints, &res) != 0) {
        return results;
    }

    std::set<std::string> uniq;
    for (auto* ptr = res; ptr != nullptr; ptr = ptr->ai_next) {
        char host[INET6_ADDRSTRLEN] = {0};
        if (ptr->ai_family == AF_INET) {
            auto* addr = reinterpret_cast<sockaddr_in*>(ptr->ai_addr);
            inet_ntop(AF_INET, &addr->sin_addr, host, sizeof(host));
        } else if (ptr->ai_family == AF_INET6) {
            auto* addr = reinterpret_cast<sockaddr_in6*>(ptr->ai_addr);
            inet_ntop(AF_INET6, &addr->sin6_addr, host, sizeof(host));
        } else {
            continue;
        }
        if (host[0]) {
            uniq.insert(host);
        }
    }
    freeaddrinfo(res);

    results.assign(uniq.begin(), uniq.end());
    return results;
}

std::string extract_registrar(const json& root) {
    if (root.contains("registrar") && root["registrar"].is_string()) {
        return root["registrar"].get<std::string>();
    }

    if (!root.contains("entities") || !root["entities"].is_array()) {
        return "";
    }

    for (const auto& entity : root["entities"]) {
        if (!entity.contains("roles") || !entity["roles"].is_array()) {
            continue;
        }
        bool is_registrar = false;
        for (const auto& role : entity["roles"]) {
            if (role.is_string() && utils::to_lower(role.get<std::string>()) == "registrar") {
                is_registrar = true;
                break;
            }
        }
        if (!is_registrar) {
            continue;
        }
        if (entity.contains("vcardArray") && entity["vcardArray"].is_array()) {
            const auto& vcard = entity["vcardArray"];
            if (vcard.size() >= 2 && vcard[1].is_array()) {
                for (const auto& item : vcard[1]) {
                    if (!item.is_array() || item.size() < 4) {
                        continue;
                    }
                    if (item[0].is_string() && item[0].get<std::string>() == "fn" && item[3].is_string()) {
                        return item[3].get<std::string>();
                    }
                }
            }
        }
    }
    return "";
}

enum class DomainRequestKind { Https, Http, Robots, Security, Ct, Rdap };

struct DomainRequest {
    DomainRequestKind kind;
    engines::HttpRequest request;
};

} // namespace

std::string normalize_domain(std::string value) {
    value = utils::trim(value);
    value = utils::to_lower(value);
    if (value.rfind("http://", 0) == 0) {
        value.erase(0, 7);
    } else if (value.rfind("https://", 0) == 0) {
        value.erase(0, 8);
    }
    auto slash = value.find('/');
    if (slash != std::string::npos) {
        value = value.substr(0, slash);
    }
    auto colon = value.find(':');
    if (colon != std::string::npos) {
        value = value.substr(0, colon);
    }
    return utils::trim(value);
}

std::vector<std::string> parse_ct_subdomains(const std::string& json_body, const std::string& domain, int max_subdomains) {
    std::set<std::string> unique;
    if (json_body.empty()) {
        return {};
    }
    try {
        auto payload = json::parse(json_body);
        if (!payload.is_array()) {
            return {};
        }
        for (const auto& entry : payload) {
            if (!entry.contains("name_value")) {
                continue;
            }
            if (!entry["name_value"].is_string()) {
                continue;
            }
            auto name_value = entry["name_value"].get<std::string>();
            auto parts = utils::split(name_value, '\n');
            for (auto& part : parts) {
                auto normalized = utils::to_lower(utils::trim(part));
                if (normalized.empty()) {
                    continue;
                }
                if (normalized == domain || normalized.size() <= domain.size()) {
                    continue;
                }
                if (normalized.size() <= domain.size() + 1) {
                    continue;
                }
                size_t pos = normalized.size() - domain.size();
                if (normalized.compare(pos, domain.size(), domain) != 0) {
                    continue;
                }
                if (pos == 0 || normalized[pos - 1] != '.') {
                    continue;
                }
                unique.insert(normalized);
                if (static_cast<int>(unique.size()) >= max_subdomains) {
                    break;
                }
            }
            if (static_cast<int>(unique.size()) >= max_subdomains) {
                break;
            }
        }
    } catch (const std::exception&) {
        return {};
    }

    return std::vector<std::string>(unique.begin(), unique.end());
}

RdapInfo parse_rdap_info(const std::string& json_body) {
    RdapInfo info;
    if (json_body.empty()) {
        return info;
    }
    try {
        auto payload = json::parse(json_body);
        if (!payload.is_object()) {
            return info;
        }
        if (payload.contains("handle") && payload["handle"].is_string()) {
            info.handle = payload["handle"].get<std::string>();
        }
        info.registrar = extract_registrar(payload);
        if (payload.contains("nameservers") && payload["nameservers"].is_array()) {
            for (const auto& ns : payload["nameservers"]) {
                if (ns.contains("ldhName") && ns["ldhName"].is_string()) {
                    info.name_servers.push_back(ns["ldhName"].get<std::string>());
                }
            }
        }
    } catch (const std::exception&) {
        return info;
    }
    return info;
}

DomainScanResult collect_domain_surface(const std::string& domain, const DomainScanOptions& options) {
    DomainScanResult result;
    result.target_domain = normalize_domain(domain);
    if (result.target_domain.empty()) {
        return result;
    }

    const std::string ct_base = options.ct_base_url.empty() ? "https://crt.sh" : options.ct_base_url;
    const std::string rdap_base = options.rdap_base_url.empty() ? "https://rdap.org" : options.rdap_base_url;

    auto resolve_task = std::async(std::launch::async, [&]() {
        return resolve_addresses(result.target_domain);
    });

    std::vector<DomainRequest> requests;
    requests.reserve(6);
    auto push_request = [&](DomainRequestKind kind, const std::string& url, size_t max_body) {
        engines::HttpRequest req;
        req.url = url;
        req.method = "GET";
        req.timeout_ms = options.timeout_ms;
        req.proxy_url = options.proxy_url;
        req.max_body_bytes = max_body;
        requests.push_back({kind, req});
    };

    const std::string target = result.target_domain;
    push_request(DomainRequestKind::Https, "https://" + target, 16384);
    push_request(DomainRequestKind::Http, "http://" + target, 16384);
    push_request(DomainRequestKind::Robots, "https://" + target + "/robots.txt", 2048);
    push_request(DomainRequestKind::Security, "https://" + target + "/.well-known/security.txt", 2048);

    if (options.include_ct) {
        std::string url = ct_base + "/?q=%25." + target + "&output=json";
        push_request(DomainRequestKind::Ct, url, 200000);
    }
    if (options.include_rdap) {
        std::string url = rdap_base + "/domain/" + target;
        push_request(DomainRequestKind::Rdap, url, 200000);
    }

    std::vector<engines::HttpRequest> async_requests;
    async_requests.reserve(requests.size());
    for (const auto& entry : requests) {
        async_requests.push_back(entry.request);
    }

    int concurrency = options.concurrency > 0 ? options.concurrency : static_cast<int>(async_requests.size());
    if (concurrency <= 0) {
        concurrency = 1;
    }
    auto responses = engines::run_async_batch(async_requests, concurrency);

    for (size_t i = 0; i < responses.size() && i < requests.size(); ++i) {
        const auto& resp = responses[i];
        switch (requests[i].kind) {
            case DomainRequestKind::Https: {
                result.https = to_artifact(resp);
                break;
            }
            case DomainRequestKind::Http: {
                result.http = to_artifact(resp);
                break;
            }
            case DomainRequestKind::Robots: {
                auto artifact = to_artifact(resp);
                result.robots_txt_present = artifact.status == 200 && !artifact.body.empty();
                if (result.robots_txt_present) {
                    result.robots_preview = artifact.body.substr(0, 512);
                }
                break;
            }
            case DomainRequestKind::Security: {
                auto artifact = to_artifact(resp);
                result.security_txt_present = artifact.status == 200 && !artifact.body.empty();
                if (result.security_txt_present) {
                    result.security_preview = artifact.body.substr(0, 512);
                }
                break;
            }
            case DomainRequestKind::Ct: {
                result.subdomains = parse_ct_subdomains(resp.body, result.target_domain, options.max_subdomains);
                break;
            }
            case DomainRequestKind::Rdap: {
                result.rdap = parse_rdap_info(resp.body);
                break;
            }
        }
    }

    result.resolved_addresses = resolve_task.get();

    auto http_loc = utils::to_lower(result.http.final_url);
    if (result.http.status >= 300 && result.http.status < 400 && http_loc.rfind("https://", 0) == 0) {
        result.http_redirects_to_https = true;
    }

    if (result.subdomains.size() > 50) {
        result.scan_notes.push_back("Broad CT host surface observed.");
    }

    return result;
}

} // namespace silicore::collect
