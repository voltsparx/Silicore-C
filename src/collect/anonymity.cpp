#include "collect/anonymity.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <cerrno>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace silicore::collect {

namespace {

std::string quote_if_needed(const std::string& value) {
    if (value.find(' ') == std::string::npos) {
        return value;
    }
    return "\"" + value + "\"";
}

std::vector<std::string> split_path_list(const std::string& value, char delim) {
    std::vector<std::string> out;
    std::string current;
    for (char ch : value) {
        if (ch == delim) {
            if (!current.empty()) {
                out.push_back(current);
            }
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    if (!current.empty()) {
        out.push_back(current);
    }
    return out;
}

std::string find_command_path(const std::string& name) {
    if (const char* env = std::getenv("PATH")) {
        char delim =
#ifdef _WIN32
            ';';
#else
            ':';
#endif
        auto parts = split_path_list(env, delim);
        for (const auto& dir : parts) {
            if (dir.empty()) {
                continue;
            }
            std::filesystem::path candidate = std::filesystem::path(dir) / name;
            if (std::filesystem::exists(candidate)) {
                return candidate.string();
            }
#ifdef _WIN32
            std::filesystem::path exe = candidate;
            exe += ".exe";
            if (std::filesystem::exists(exe)) {
                return exe.string();
            }
#endif
        }
    }
    return "";
}

std::string resolve_tor_binary() {
    if (const char* env = std::getenv("SILICORE_TOR_BIN")) {
        if (*env) {
            return std::string(env);
        }
    }
    auto from_path = find_command_path("tor");
    if (!from_path.empty()) {
        return from_path;
    }
#ifdef _WIN32
    const char* pf = std::getenv("ProgramFiles");
    const char* pfx86 = std::getenv("ProgramFiles(x86)");
    const char* local = std::getenv("LocalAppData");
    std::vector<std::string> candidates;
    if (pf) {
        candidates.push_back(std::string(pf) + "\\Tor Browser\\Browser\\TorBrowser\\Tor\\tor.exe");
    }
    if (pfx86) {
        candidates.push_back(std::string(pfx86) + "\\Tor Browser\\Browser\\TorBrowser\\Tor\\tor.exe");
    }
    if (local) {
        candidates.push_back(std::string(local) + "\\Tor Browser\\Browser\\TorBrowser\\Tor\\tor.exe");
    }
    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }
#endif
    return "";
}

bool command_exists(const std::string& name) {
    return !find_command_path(name).empty();
}

#ifdef _WIN32
bool ensure_winsock() {
    static bool initialized = false;
    if (initialized) {
        return true;
    }
    WSADATA data;
    if (WSAStartup(MAKEWORD(2, 2), &data) == 0) {
        initialized = true;
        return true;
    }
    return false;
}
#endif

bool port_open(const std::string& host, int port, int timeout_ms) {
    if (timeout_ms <= 0) {
        timeout_ms = 600;
    }
#ifdef _WIN32
    if (!ensure_winsock()) {
        return false;
    }
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        return false;
    }
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<u_short>(port));
    inet_pton(AF_INET, host.c_str(), &addr.sin_addr);
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);
    int rc = connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if (rc == 0) {
        closesocket(sock);
        return true;
    }
    int err = WSAGetLastError();
    if (err != WSAEWOULDBLOCK && err != WSAEINPROGRESS && err != WSAEINVAL) {
        closesocket(sock);
        return false;
    }
    fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(sock, &wfds);
    timeval tv{};
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    rc = select(0, nullptr, &wfds, nullptr, &tv);
    if (rc > 0 && FD_ISSET(sock, &wfds)) {
        int so_error = 0;
        int len = sizeof(so_error);
        getsockopt(sock, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&so_error), &len);
        closesocket(sock);
        return so_error == 0;
    }
    closesocket(sock);
    return false;
#else
    int sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return false;
    }
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags >= 0) {
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    }
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<u_short>(port));
    inet_pton(AF_INET, host.c_str(), &addr.sin_addr);
    int rc = ::connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if (rc == 0) {
        close(sock);
        return true;
    }
    if (errno != EINPROGRESS) {
        close(sock);
        return false;
    }
    fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(sock, &wfds);
    timeval tv{};
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    rc = select(sock + 1, nullptr, &wfds, nullptr, &tv);
    if (rc > 0 && FD_ISSET(sock, &wfds)) {
        int so_error = 0;
        socklen_t len = sizeof(so_error);
        getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len);
        close(sock);
        return so_error == 0;
    }
    close(sock);
    return false;
#endif
}

std::filesystem::path tor_data_dir() {
    if (const char* env = std::getenv("SILICORE_TOR_DATA_DIR")) {
        if (*env) {
            return std::filesystem::path(env);
        }
    }
#ifdef _WIN32
    if (const char* temp = std::getenv("TEMP")) {
        return std::filesystem::path(temp) / "silicore-c-tor";
    }
    return std::filesystem::temp_directory_path() / "silicore-c-tor";
#else
    if (const char* xdg = std::getenv("XDG_STATE_HOME")) {
        return std::filesystem::path(xdg) / "silicore-c" / "tor";
    }
    if (const char* home = std::getenv("HOME")) {
        return std::filesystem::path(home) / ".local" / "state" / "silicore-c" / "tor";
    }
    return std::filesystem::temp_directory_path() / "silicore-c-tor";
#endif
}

std::filesystem::path tor_config_path(const std::filesystem::path& data_dir) {
    return data_dir / "torrc.silicore";
}

bool write_tor_config(const std::filesystem::path& data_dir) {
    std::error_code ec;
    std::filesystem::create_directories(data_dir, ec);
    if (ec) {
        return false;
    }
    auto config = tor_config_path(data_dir);
    if (std::filesystem::exists(config)) {
        return true;
    }
    std::ofstream out(config);
    if (!out.is_open()) {
        return false;
    }
    auto data_str = data_dir.string();
    auto log_path = (data_dir / "tor.log").string();
    out << "SocksPort 9050\n";
    out << "DataDirectory " << quote_if_needed(data_str) << "\n";
    out << "Log notice file " << quote_if_needed(log_path) << "\n";
    return true;
}

bool run_command(const std::string& cmd) {
    int rc = std::system(cmd.c_str());
    return rc == 0;
}

bool install_tor() {
#ifdef _WIN32
    if (command_exists("winget")) {
        return run_command("winget install -e --id TorProject.TorBrowser --accept-source-agreements --accept-package-agreements");
    }
    if (command_exists("choco")) {
        return run_command("choco install -y tor-browser");
    }
    return false;
#elif defined(__APPLE__)
    if (command_exists("brew")) {
        return run_command("brew install tor");
    }
    return false;
#else
    if (command_exists("pkg")) {
        return run_command("pkg install -y tor");
    }
    std::string sudo;
    if (geteuid() != 0 && command_exists("sudo")) {
        sudo = "sudo ";
    }
    if (command_exists("apt-get")) {
        return run_command(sudo + "apt-get update && " + sudo + "apt-get install -y tor");
    }
    if (command_exists("dnf")) {
        return run_command(sudo + "dnf install -y tor");
    }
    if (command_exists("yum")) {
        return run_command(sudo + "yum install -y tor");
    }
    if (command_exists("pacman")) {
        return run_command(sudo + "pacman -Sy --noconfirm tor");
    }
    if (command_exists("zypper")) {
        return run_command(sudo + "zypper --non-interactive install tor");
    }
    return false;
#endif
}

bool start_tor() {
    auto tor_bin = resolve_tor_binary();
    if (tor_bin.empty()) {
        return false;
    }
    auto data_dir = tor_data_dir();
    if (!write_tor_config(data_dir)) {
        return false;
    }
    auto config = tor_config_path(data_dir);
#ifdef _WIN32
    std::ostringstream cmd;
    cmd << "start \"\" /B " << quote_if_needed(tor_bin)
        << " -f " << quote_if_needed(config.string());
    return run_command(cmd.str());
#else
    std::string sudo;
    if (geteuid() != 0 && command_exists("sudo")) {
        sudo = "sudo ";
    }
    if (command_exists("systemctl")) {
        if (run_command(sudo + "systemctl start tor")) {
            return true;
        }
    }
    if (command_exists("service")) {
        if (run_command(sudo + "service tor start")) {
            return true;
        }
    }
    std::ostringstream cmd;
    cmd << quote_if_needed(tor_bin)
        << " -f " << quote_if_needed(config.string())
        << " --RunAsDaemon 1";
    return run_command(cmd.str());
#endif
}

} // namespace

std::string tor_proxy_url() {
    if (const char* env = std::getenv("SILICORE_TOR_PROXY")) {
        if (*env) {
            return std::string(env);
        }
    }
    return "socks5h://127.0.0.1:9050";
}

bool is_tor_running(int timeout_ms) {
    return port_open("127.0.0.1", 9050, timeout_ms);
}

bool ensure_tor_running(bool allow_install) {
    if (is_tor_running()) {
        return true;
    }
    if (resolve_tor_binary().empty()) {
        if (!allow_install) {
            return false;
        }
        if (!install_tor()) {
            return false;
        }
    }
    if (!start_tor()) {
        return false;
    }
    for (int i = 0; i < 10; ++i) {
        if (is_tor_running(400)) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
    return false;
}

} // namespace silicore::collect
