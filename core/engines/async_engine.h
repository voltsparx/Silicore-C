#pragma once

#include <string>
#include <vector>

namespace silicore::engines {

struct HttpRequest {
    std::string url;
    std::string method;   // GET | HEAD
    int timeout_ms = 20000;
    std::string proxy_url;
};

struct HttpResponse {
    int status_code = -1;
    std::string body;
    std::string final_url;
    long elapsed_ms = 0;
    std::string error;
};

std::vector<HttpResponse> run_async_batch(
    const std::vector<HttpRequest>& requests,
    int concurrency_limit = 200,
    size_t max_body_bytes = 65536
);

} // namespace silicore::engines
