#pragma once

#include <filesystem>
#include <memory>
#include <string>

namespace silicore::interface {

class LiveServer {
public:
    LiveServer();
    ~LiveServer();

    LiveServer(const LiveServer&) = delete;
    LiveServer& operator=(const LiveServer&) = delete;
    LiveServer(LiveServer&&) noexcept;
    LiveServer& operator=(LiveServer&&) noexcept;

    bool start(const std::filesystem::path& root, int port);
    void stop();
    bool running() const;
    int port() const;
    std::filesystem::path root() const;
    std::string base_url() const;
    std::string url_for(const std::string& relative_path) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace silicore::interface
