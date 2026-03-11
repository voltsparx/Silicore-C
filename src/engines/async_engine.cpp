#include "engines/async_engine.h"

#include "foundation/metadata.h"
#include "utils/strings.h"

#include <curl/curl.h>
#include <chrono>
#include <deque>
#include <memory>
#include <unordered_map>

namespace silicore::engines {

namespace {

struct CurlGlobal {
    CurlGlobal() { curl_global_init(CURL_GLOBAL_ALL); }
    ~CurlGlobal() { curl_global_cleanup(); }
};

CurlGlobal& ensure_curl_global() {
    static CurlGlobal global;
    return global;
}

struct CurlContext {
    size_t index = 0;
    HttpResponse response;
    std::chrono::steady_clock::time_point start;
    std::string header_buffer;
    size_t max_body = 0;
    char error_buffer[CURL_ERROR_SIZE] = {0};
    std::string user_agent;
};

size_t write_body(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* ctx = static_cast<CurlContext*>(userdata);
    const size_t total = size * nmemb;
    if (ctx->response.body.size() >= ctx->max_body) {
        return total;
    }
    const size_t remaining = ctx->max_body - ctx->response.body.size();
    const size_t to_copy = total < remaining ? total : remaining;
    ctx->response.body.append(ptr, to_copy);
    return total;
}

size_t write_header(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* ctx = static_cast<CurlContext*>(userdata);
    const size_t total = size * nmemb;
    ctx->header_buffer.append(ptr, total);
    return total;
}

void setup_easy(CURL* easy, const HttpRequest& req, CurlContext* ctx) {
    curl_easy_setopt(easy, CURLOPT_URL, req.url.c_str());
    curl_easy_setopt(easy, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(easy, CURLOPT_MAXREDIRS, 5L);
    curl_easy_setopt(easy, CURLOPT_TIMEOUT_MS, req.timeout_ms);
    curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, write_body);
    curl_easy_setopt(easy, CURLOPT_WRITEDATA, ctx);
    curl_easy_setopt(easy, CURLOPT_HEADERFUNCTION, write_header);
    curl_easy_setopt(easy, CURLOPT_HEADERDATA, ctx);
    curl_easy_setopt(easy, CURLOPT_ERRORBUFFER, ctx->error_buffer);
    curl_easy_setopt(easy, CURLOPT_PRIVATE, ctx);
    ctx->user_agent = std::string(foundation::PROJECT_NAME) + "/" + foundation::VERSION +
        " (" + foundation::VERSION_THEME + ")";
    curl_easy_setopt(easy, CURLOPT_USERAGENT, ctx->user_agent.c_str());

    if (!req.proxy_url.empty()) {
        curl_easy_setopt(easy, CURLOPT_PROXY, req.proxy_url.c_str());
    }

    if (req.method == "HEAD") {
        curl_easy_setopt(easy, CURLOPT_NOBODY, 1L);
    } else {
        curl_easy_setopt(easy, CURLOPT_HTTPGET, 1L);
    }
}

std::unordered_map<std::string, std::string> parse_headers(const std::string& buffer) {
    std::unordered_map<std::string, std::string> headers;
    size_t start = 0;
    while (start < buffer.size()) {
        size_t end = buffer.find("\r\n", start);
        if (end == std::string::npos) {
            end = buffer.size();
        }
        std::string line = buffer.substr(start, end - start);
        start = end + 2;
        if (line.empty() || line.find(':') == std::string::npos) {
            continue;
        }
        auto pos = line.find(':');
        std::string key = utils::to_lower(utils::trim(line.substr(0, pos)));
        std::string val = utils::trim(line.substr(pos + 1));
        if (!key.empty()) {
            headers[key] = val;
        }
    }
    return headers;
}

} // namespace

std::vector<HttpResponse> run_async_batch(
    const std::vector<HttpRequest>& requests,
    int concurrency_limit,
    size_t max_body_bytes
) {
    std::vector<HttpResponse> results(requests.size());
    if (requests.empty()) {
        return results;
    }
    if (concurrency_limit <= 0) {
        concurrency_limit = 1;
    }

    ensure_curl_global();

    CURLM* multi = curl_multi_init();
    std::deque<size_t> pending;
    for (size_t i = 0; i < requests.size(); ++i) {
        pending.push_back(i);
    }

    std::vector<std::unique_ptr<CurlContext>> contexts(requests.size());

    auto add_handle = [&](size_t idx) {
        CURL* easy = curl_easy_init();
        auto ctx = std::make_unique<CurlContext>();
        ctx->index = idx;
        ctx->start = std::chrono::steady_clock::now();
        const auto& req = requests[idx];
        ctx->max_body = req.max_body_bytes > 0 ? req.max_body_bytes : max_body_bytes;
        setup_easy(easy, req, ctx.get());
        contexts[idx] = std::move(ctx);
        curl_multi_add_handle(multi, easy);
    };

    int count = 0;
    while (count < concurrency_limit && !pending.empty()) {
        auto idx = pending.front();
        pending.pop_front();
        add_handle(idx);
        count++;
    }

    int still_running = 0;
    curl_multi_perform(multi, &still_running);

    while (still_running > 0) {
        int numfds = 0;
        curl_multi_wait(multi, nullptr, 0, 1000, &numfds);
        curl_multi_perform(multi, &still_running);

        int msgs = 0;
        CURLMsg* msg = nullptr;
        while ((msg = curl_multi_info_read(multi, &msgs))) {
            if (msg->msg != CURLMSG_DONE) {
                continue;
            }

            CURL* easy = msg->easy_handle;
            CurlContext* ctx = nullptr;
            curl_easy_getinfo(easy, CURLINFO_PRIVATE, &ctx);

            if (ctx) {
                long code = 0;
                char* effective_url = nullptr;
                curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &code);
                curl_easy_getinfo(easy, CURLINFO_EFFECTIVE_URL, &effective_url);

                ctx->response.status_code = static_cast<int>(code);
                if (effective_url) {
                    ctx->response.final_url = effective_url;
                }
                if (msg->data.result != CURLE_OK) {
                    ctx->response.error = ctx->error_buffer[0] ? ctx->error_buffer : curl_easy_strerror(msg->data.result);
                }
                ctx->response.headers = parse_headers(ctx->header_buffer);
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - ctx->start
                );
                ctx->response.elapsed_ms = elapsed.count();
                results[ctx->index] = ctx->response;
            }

            curl_multi_remove_handle(multi, easy);
            curl_easy_cleanup(easy);

            if (!pending.empty()) {
                auto idx = pending.front();
                pending.pop_front();
                add_handle(idx);
                curl_multi_perform(multi, &still_running);
            }
        }
    }

    curl_multi_cleanup(multi);

    return results;
}

} // namespace silicore::engines

