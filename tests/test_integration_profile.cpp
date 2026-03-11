#include "core/collect/platform_scanner.h"

#include <gtest/gtest.h>

#include <atomic>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>

using namespace silicore::collect;

namespace {

struct Response {
    int status = 200;
    std::string body = "ok";
};

class TestHttpServer {
public:
    TestHttpServer() = default;
    ~TestHttpServer() { stop(); }

    void set_route(const std::string& path, const Response& resp) { routes_[path] = resp; }

    void start() {
        server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        int opt = 1;
        setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = 0;
        bind(server_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));

        socklen_t len = sizeof(addr);
        getsockname(server_fd_, reinterpret_cast<sockaddr*>(&addr), &len);
        port_ = ntohs(addr.sin_port);

        listen(server_fd_, 8);
        running_.store(true);
        thread_ = std::thread([this]() { loop(); });
    }

    void stop() {
        if (!running_.exchange(false)) {
            return;
        }
        if (server_fd_ >= 0) {
            shutdown(server_fd_, SHUT_RDWR);
            close(server_fd_);
            server_fd_ = -1;
        }
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    int port() const { return port_; }

private:
    void loop() {
        while (running_.load()) {
            sockaddr_in client{};
            socklen_t len = sizeof(client);
            int client_fd = accept(server_fd_, reinterpret_cast<sockaddr*>(&client), &len);
            if (client_fd < 0) {
                continue;
            }
            char buffer[1024] = {0};
            ssize_t read_bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
            std::string request = read_bytes > 0 ? std::string(buffer, read_bytes) : "";
            std::string path = "/";
            auto first_space = request.find(' ');
            if (first_space != std::string::npos) {
                auto second_space = request.find(' ', first_space + 1);
                if (second_space != std::string::npos) {
                    path = request.substr(first_space + 1, second_space - first_space - 1);
                }
            }

            Response resp = default_response_;
            if (routes_.count(path)) {
                resp = routes_[path];
            }

            std::string status_text = resp.status == 200 ? "OK" : "Not Found";
            std::string body = resp.body;
            std::string response = "HTTP/1.1 " + std::to_string(resp.status) + " " + status_text + "\r\n";
            response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
            response += "Connection: close\r\n\r\n";
            response += body;
            send(client_fd, response.c_str(), response.size(), 0);
            close(client_fd);
        }
    }

    int server_fd_ = -1;
    int port_ = 0;
    std::atomic<bool> running_{false};
    std::thread thread_;
    std::unordered_map<std::string, Response> routes_;
    Response default_response_;
};

} // namespace

TEST(ProfileIntegration, ScansLocalServer) {
    TestHttpServer server;
    server.set_route("/alice", {200, "ok"});
    server.set_route("/missing", {404, "not found"});
    server.start();

    PlatformConfig cfg;
    cfg.name = "Local";
    cfg.url = "http://127.0.0.1:" + std::to_string(server.port()) + "/{username}";
    cfg.url_probe = cfg.url;
    cfg.detection_methods = {"status_code"};
    cfg.exists_statuses = {200};
    cfg.not_found_statuses = {404};
    cfg.request_method = "GET";

    PlatformScanner scanner({cfg});
    auto result = scanner.scan("alice", 1, 2000, "");
    ASSERT_EQ(result.profiles.size(), 1u);
    EXPECT_EQ(result.profiles[0].status, "FOUND");

    auto result2 = scanner.scan("missing", 1, 2000, "");
    ASSERT_EQ(result2.profiles.size(), 1u);
    EXPECT_EQ(result2.profiles[0].status, "NOT_FOUND");
}
