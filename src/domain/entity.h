#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace silicore::domain {

enum class EntityType {
    Base,
    Profile,
    Domain,
    Asset,
    Ip
};

struct BaseEntity {
    std::string id;
    std::string value;
    std::string source;
    float confidence = 0.0f;
    EntityType entity_type = EntityType::Base;
    std::chrono::system_clock::time_point timestamp;
    std::unordered_map<std::string, std::string> attributes;
    std::vector<std::string> relationships;
};

struct ContactInfo {
    std::vector<std::string> emails;
    std::vector<std::string> phones;
};

struct ProfileEntity : BaseEntity {
    std::string platform;
    std::string profile_url;
    std::string status;
    int http_status = 0;
    long response_time_ms = 0;
    std::string context;
    std::string bio;
    std::vector<std::string> links;
    std::vector<std::string> mentions;
    ContactInfo contacts;
};

struct DomainEntity : BaseEntity {
    std::string domain;
    std::vector<std::string> subdomains;
};

struct AssetEntity : BaseEntity {
    std::string asset_kind;
};

struct IpEntity : BaseEntity {
    std::string ip_version;
};

std::string make_id(std::string_view kind, std::string_view source, std::string_view value);

} // namespace silicore::domain
