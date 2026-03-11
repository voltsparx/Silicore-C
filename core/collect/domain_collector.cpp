#include "core/collect/domain_collector.h"

#include "core/utils/strings.h"

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <future>
#include <netdb.h>
#include <set>
#include <sys/socket.h>
#include <arpa/inet.h>

namespace silicore::collect {

using json = nlohmann::json;

namespace {

struct CurlGlobal {
    CurlGlobal() { curl_global_init(CURL_GLOBAL_ALL); }
    ~CurlGlobal() { curl_global_cleanup(); }
};

CurlGlobal& curl_global() {
    static CurlGlobal global;
    return global;
}

size_t write_body(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* body = static_cast<std::string*>(userdata);
    body->append(ptr, size * nmemb);
    return size * nmemb;
}

size_t write_header(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* headers = static_cast<std::unordered_map<std::string, std::string>*>(userdata);
    std::string line(ptr, size * nmemb);
    auto pos = line.find(':');
    if (pos != std::string::npos) {
        auto key = utils::to_lower(utils::trim(line.substr(0, pos)));
        auto val = utils::trim(line.substr(pos + 1));
        if (!key.empty()) {
            (*headers)[key] = val;
        }
    }
    return size * nmemb;
}

HttpArtifact http_get(const std::string& url, int timeout_ms, const std::string& proxy_url, size_t max_body = 65536) {
    curl_global();

    HttpArtifact artifact;
    auto start = std::chrono::steady_clock::now();

    CURL* easy = curl_easy_init();
    if (!easy) {
        artifact.error = "curl_init_failed";
        return artifact;
    }

    std::string body;
    std::unordered_map<std::string, std::string> headers;

    curl_easy_setopt(easy, CURLOPT_URL, url.c_str());
    curl_easy_setopt(easy, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(easy, CURLOPT_MAXREDIRS, 5L);
    curl_easy_setopt(easy, CURLOPT_TIMEOUT_MS, timeout_ms);
    curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, write_body);
    curl_easy_setopt(easy, CURLOPT_WRITEDATA, &body);
    curl_easy_setopt(easy, CURLOPT_HEADERFUNCTION, write_header);
    curl_easy_setopt(easy, CURLOPT_HEADERDATA, &headers);
    curl_easy_setopt(easy, CURLOPT_USERAGENT, "Silicore-C/1.0");

    if (!proxy_url.empty()) {
        curl_easy_setopt(easy, CURLOPT_PROXY, proxy_url.c_str());
    }

    CURLcode res = curl_easy_perform(easy);

    long status = 0;
    char* effective = nullptr;
    curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &status);
    curl_easy_getinfo(easy, CURLINFO_EFFECTIVE_URL, &effective);

    artifact.status = static_cast<int>(status);
    artifact.final_url = effective ? effective : url;
    artifact.headers = std::move(headers);

    if (res != CURLE_OK) {
        artifact.error = curl_easy_strerror(res);
    }

    if (body.size() > max_body) {
        body.resize(max_body);
    }
    artifact.body = std::move(body);

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start
    );
    artifact.elapsed_ms = elapsed.count();

    curl_easy_cleanup(easy);
    return artifact;
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

    auto https_task = std::async(std::launch::async, [&]() {
        return http_get("https://" + result.target_domain, options.timeout_ms, options.proxy_url, 16384);
    });

    auto http_task = std::async(std::launch::async, [&]() {
        return http_get("http://" + result.target_domain, options.timeout_ms, options.proxy_url, 16384);
    });

    auto robots_task = std::async(std::launch::async, [&]() {
        return http_get("https://" + result.target_domain + "/robots.txt", options.timeout_ms, options.proxy_url, 2048);
    });

    auto security_task = std::async(std::launch::async, [&]() {
        return http_get("https://" + result.target_domain + "/.well-known/security.txt", options.timeout_ms, options.proxy_url, 2048);
    });

    std::future<std::vector<std::string>> ct_task;
    if (options.include_ct) {
        ct_task = std::async(std::launch::async, [&]() {
            auto url = ct_base + "/?q=%25." + result.target_domain + "&output=json";
            auto ct_resp = http_get(url, options.timeout_ms, options.proxy_url, 200000);
            return parse_ct_subdomains(ct_resp.body, result.target_domain, options.max_subdomains);
        });
    }

    std::future<RdapInfo> rdap_task;
    if (options.include_rdap) {
        rdap_task = std::async(std::launch::async, [&]() {
            auto url = rdap_base + "/domain/" + result.target_domain;
            auto rdap_resp = http_get(url, options.timeout_ms, options.proxy_url, 200000);
            return parse_rdap_info(rdap_resp.body);
        });
    }

    result.resolved_addresses = resolve_task.get();
    result.https = https_task.get();
    result.http = http_task.get();
    auto robots = robots_task.get();
    auto security = security_task.get();

    if (options.include_ct) {
        result.subdomains = ct_task.get();
    }
    if (options.include_rdap) {
        result.rdap = rdap_task.get();
    }

    result.robots_txt_present = robots.status == 200 && !robots.body.empty();
    if (result.robots_txt_present) {
        result.robots_preview = robots.body.substr(0, 512);
    }

    result.security_txt_present = security.status == 200 && !security.body.empty();
    if (result.security_txt_present) {
        result.security_preview = security.body.substr(0, 512);
    }

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
