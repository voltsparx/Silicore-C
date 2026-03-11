#include "interface/live_server.h"

#include <atomic>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace silicore::interface {

namespace {

constexpr int kSelectTimeoutMs = 200;
constexpr size_t kMaxRequestBytes = 8192;

#ifdef _WIN32
using SocketHandle = SOCKET;
constexpr SocketHandle kInvalidSocket = INVALID_SOCKET;
#else
using SocketHandle = int;
constexpr SocketHandle kInvalidSocket = -1;
#endif

void close_socket(SocketHandle socket) {
    if (socket == kInvalidSocket) {
        return;
    }
#ifdef _WIN32
    closesocket(socket);
#else
    close(socket);
#endif
}

bool send_all(SocketHandle socket, const char* data, size_t length) {
    size_t sent = 0;
    while (sent < length) {
#ifdef _WIN32
        int chunk = ::send(socket, data + sent, static_cast<int>(length - sent), 0);
#else
        ssize_t chunk = ::send(socket, data + sent, length - sent, 0);
#endif
        if (chunk <= 0) {
            return false;
        }
        sent += static_cast<size_t>(chunk);
    }
    return true;
}

std::string to_lower(std::string value) {
    for (auto& ch : value) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return value;
}

std::string content_type_for(const std::filesystem::path& path) {
    auto ext = to_lower(path.extension().string());
    if (ext == ".html" || ext == ".htm") return "text/html; charset=utf-8";
    if (ext == ".css") return "text/css; charset=utf-8";
    if (ext == ".js") return "text/javascript; charset=utf-8";
    if (ext == ".json") return "application/json; charset=utf-8";
    if (ext == ".txt") return "text/plain; charset=utf-8";
    if (ext == ".svg") return "image/svg+xml";
    if (ext == ".png") return "image/png";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    return "application/octet-stream";
}

std::string html_escape(const std::string& input) {
    std::string out;
    out.reserve(input.size());
    for (char ch : input) {
        switch (ch) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            case '\'': out += "&#39;"; break;
            default: out.push_back(ch); break;
        }
    }
    return out;
}

bool build_relative_path(const std::string& raw_path, std::filesystem::path& relative) {
    if (raw_path.empty() || raw_path[0] != '/') {
        return false;
    }
    std::string trimmed = raw_path;
    auto query_pos = trimmed.find('?');
    if (query_pos != std::string::npos) {
        trimmed = trimmed.substr(0, query_pos);
    }
    if (trimmed == "/") {
        relative.clear();
        return true;
    }
    std::string rel = trimmed.substr(1);
    if (rel.empty()) {
        relative.clear();
        return true;
    }
    std::filesystem::path out;
    std::string segment;
    std::istringstream iss(rel);
    while (std::getline(iss, segment, '/')) {
        if (segment.empty() || segment == ".") {
            continue;
        }
        if (segment == "..") {
            return false;
        }
        out /= segment;
    }
    relative = out;
    return true;
}

std::string build_index_page(const std::filesystem::path& root) {
    std::vector<std::string> entries;
    std::error_code ec;
    if (std::filesystem::is_directory(root, ec)) {
        for (const auto& entry : std::filesystem::directory_iterator(root, ec)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            if (entry.path().extension() == ".html") {
                entries.push_back(entry.path().filename().string());
            }
        }
    }
    std::ostringstream out;
    out << "<!doctype html><html lang='en'><head><meta charset='utf-8'>"
           "<meta name='viewport' content='width=device-width, initial-scale=1'>"
           "<title>Silicore-C Live Reports</title>"
           "<style>"
           "body{font-family:Arial,Helvetica,sans-serif;background:#0f1720;color:#e2e8f0;margin:0;padding:24px;}"
           ".card{background:#162231;border:1px solid #1f2f44;border-radius:12px;padding:16px;max-width:900px;margin:0 auto;}"
           "a{color:#87ceeb;text-decoration:none;}"
           "li{margin:6px 0;}"
           "</style></head><body><div class='card'>"
           "<h1>Silicore-C Live Reports</h1>";
    if (entries.empty()) {
        out << "<p>No HTML reports found in this output directory.</p>";
    } else {
        out << "<ul>";
        for (const auto& name : entries) {
            out << "<li><a href='/" << html_escape(name) << "'>" << html_escape(name) << "</a></li>";
        }
        out << "</ul>";
    }
    out << "</div></body></html>";
    return out.str();
}

} // namespace

struct LiveServer::Impl {
    std::filesystem::path root;
    std::thread thread;
    std::atomic<bool> running{false};
    std::atomic<bool> stop_requested{false};
    SocketHandle listen_socket = kInvalidSocket;
    int port = 0;
#ifdef _WIN32
    bool wsa_ready = false;
#endif
    std::mutex mutex;

    bool start(const std::filesystem::path& root_path, int requested_port) {
        std::lock_guard<std::mutex> lock(mutex);
        stop_locked();

        root = root_path;
        if (requested_port <= 0) {
            requested_port = 0;
        }

#ifdef _WIN32
        if (!wsa_ready) {
            WSADATA wsa_data{};
            if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
                return false;
            }
            wsa_ready = true;
        }
#endif

        listen_socket = ::socket(AF_INET, SOCK_STREAM, 0);
        if (listen_socket == kInvalidSocket) {
            return false;
        }

        int opt = 1;
#ifdef _WIN32
        setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));
#else
        setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<uint16_t>(requested_port));
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

        if (::bind(listen_socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
            close_socket(listen_socket);
            listen_socket = kInvalidSocket;
            return false;
        }

        if (::listen(listen_socket, SOMAXCONN) != 0) {
            close_socket(listen_socket);
            listen_socket = kInvalidSocket;
            return false;
        }

        sockaddr_in bound_addr{};
        socklen_t bound_len = sizeof(bound_addr);
        if (::getsockname(listen_socket, reinterpret_cast<sockaddr*>(&bound_addr), &bound_len) == 0) {
            port = ntohs(bound_addr.sin_port);
        } else {
            port = requested_port;
        }

        stop_requested = false;
        running.store(true);
        thread = std::thread([this]() { run(); });
        return true;
    }

    void stop() {
        std::lock_guard<std::mutex> lock(mutex);
        stop_locked();
    }

    void stop_locked() {
        if (!running.load()) {
            return;
        }
        stop_requested.store(true);
        if (thread.joinable()) {
            thread.join();
        }
        close_socket(listen_socket);
        listen_socket = kInvalidSocket;
        running.store(false);
        stop_requested.store(false);
#ifdef _WIN32
        if (wsa_ready) {
            WSACleanup();
            wsa_ready = false;
        }
#endif
    }

    void run() {
        while (!stop_requested.load()) {
            fd_set read_set;
            FD_ZERO(&read_set);
            FD_SET(listen_socket, &read_set);
            timeval tv{};
            tv.tv_sec = 0;
            tv.tv_usec = kSelectTimeoutMs * 1000;
#ifdef _WIN32
            int ready = ::select(0, &read_set, nullptr, nullptr, &tv);
#else
            int ready = ::select(listen_socket + 1, &read_set, nullptr, nullptr, &tv);
#endif
            if (ready <= 0) {
                continue;
            }
            if (!FD_ISSET(listen_socket, &read_set)) {
                continue;
            }
            sockaddr_in client_addr{};
            socklen_t client_len = sizeof(client_addr);
            SocketHandle client = ::accept(listen_socket, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
            if (client == kInvalidSocket) {
                continue;
            }
            handle_client(client);
            close_socket(client);
        }
    }

    void handle_client(SocketHandle client) {
        char buffer[kMaxRequestBytes + 1];
#ifdef _WIN32
        int received = ::recv(client, buffer, static_cast<int>(kMaxRequestBytes), 0);
#else
        ssize_t received = ::recv(client, buffer, kMaxRequestBytes, 0);
#endif
        if (received <= 0) {
            return;
        }
        buffer[received] = '\0';
        std::string request(buffer, buffer + received);
        auto line_end = request.find("\r\n");
        if (line_end == std::string::npos) {
            line_end = request.find('\n');
        }
        if (line_end == std::string::npos) {
            send_error(client, "400 Bad Request", "Malformed request.");
            return;
        }
        std::string line = request.substr(0, line_end);
        std::istringstream iss(line);
        std::string method;
        std::string path;
        iss >> method >> path;
        if (method.empty() || path.empty()) {
            send_error(client, "400 Bad Request", "Malformed request.");
            return;
        }
        bool is_head = false;
        if (method == "HEAD") {
            is_head = true;
        } else if (method != "GET") {
            send_error(client, "405 Method Not Allowed", "Only GET and HEAD are supported.");
            return;
        }

        std::filesystem::path relative;
        if (!build_relative_path(path, relative)) {
            send_error(client, "400 Bad Request", "Invalid path.");
            return;
        }

        if (relative.empty()) {
            std::filesystem::path index_path = root / "index.html";
            std::error_code ec;
            if (std::filesystem::is_regular_file(index_path, ec)) {
                send_file(client, index_path, is_head);
            } else {
                std::string body = build_index_page(root);
                send_response(client, "200 OK", "text/html; charset=utf-8", body, is_head);
            }
            return;
        }

        std::filesystem::path full_path = root / relative;
        std::error_code file_ec;
        if (!std::filesystem::is_regular_file(full_path, file_ec)) {
            send_error(client, "404 Not Found", "Report not found.");
            return;
        }
        send_file(client, full_path, is_head);
    }

    void send_response(SocketHandle client, const std::string& status, const std::string& content_type, const std::string& body, bool head_only) {
        std::ostringstream header;
        header << "HTTP/1.1 " << status << "\r\n";
        header << "Content-Type: " << content_type << "\r\n";
        header << "Content-Length: " << body.size() << "\r\n";
        header << "Cache-Control: no-store\r\n";
        header << "Connection: close\r\n\r\n";
        auto header_str = header.str();
        send_all(client, header_str.data(), header_str.size());
        if (!head_only && !body.empty()) {
            send_all(client, body.data(), body.size());
        }
    }

    void send_error(SocketHandle client, const std::string& status, const std::string& message) {
        std::ostringstream body;
        body << "<!doctype html><html><head><meta charset='utf-8'>"
             << "<title>" << status << "</title></head><body>"
             << "<h1>" << status << "</h1>"
             << "<p>" << html_escape(message) << "</p>"
             << "</body></html>";
        send_response(client, status, "text/html; charset=utf-8", body.str(), false);
    }

    void send_file(SocketHandle client, const std::filesystem::path& path, bool head_only) {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            send_error(client, "404 Not Found", "Report not found.");
            return;
        }
        std::ostringstream content;
        content << file.rdbuf();
        std::string body = content.str();
        send_response(client, "200 OK", content_type_for(path), body, head_only);
    }
};

LiveServer::LiveServer() : impl_(std::make_unique<Impl>()) {}

LiveServer::~LiveServer() {
    stop();
}

LiveServer::LiveServer(LiveServer&& other) noexcept = default;
LiveServer& LiveServer::operator=(LiveServer&& other) noexcept = default;

bool LiveServer::start(const std::filesystem::path& root, int port) {
    return impl_ && impl_->start(root, port);
}

void LiveServer::stop() {
    if (impl_) {
        impl_->stop();
    }
}

bool LiveServer::running() const {
    return impl_ && impl_->running.load();
}

int LiveServer::port() const {
    return impl_ ? impl_->port : 0;
}

std::filesystem::path LiveServer::root() const {
    return impl_ ? impl_->root : std::filesystem::path{};
}

std::string LiveServer::base_url() const {
    if (!impl_ || impl_->port <= 0) {
        return "";
    }
    return "http://127.0.0.1:" + std::to_string(impl_->port);
}

std::string LiveServer::url_for(const std::string& relative_path) const {
    auto base = base_url();
    if (base.empty()) {
        return "";
    }
    if (relative_path.empty()) {
        return base + "/";
    }
    if (relative_path.front() == '/') {
        return base + relative_path;
    }
    return base + "/" + relative_path;
}

} // namespace silicore::interface
