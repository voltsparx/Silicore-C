#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace silicore::engines {

struct HttpRequest {
    std::string url;
    std::string method;   // GET | HEAD
    int timeout_ms = 20000;
    std::string proxy_url;
    size_t max_body_bytes = 0;
};

struct HttpResponse {
    int status_code = -1;
    std::string body;
    std::string final_url;
    long elapsed_ms = 0;
    std::string error;
    std::unordered_map<std::string, std::string> headers;
};

std::vector<HttpResponse> run_async_batch(
    const std::vector<HttpRequest>& requests,
    int concurrency_limit = 200,
    size_t max_body_bytes = 65536
);

} // namespace silicore::engines
