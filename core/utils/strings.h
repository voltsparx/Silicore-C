#pragma once

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace silicore::utils {

inline std::string ltrim(std::string s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    return s;
}

inline std::string rtrim(std::string s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    return s;
}

inline std::string trim(std::string s) {
    return rtrim(ltrim(std::move(s)));
}

inline std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

inline bool contains_case_insensitive(std::string_view haystack, std::string_view needle) {
    if (needle.empty()) {
        return true;
    }
    std::string hay = to_lower(std::string(haystack));
    std::string ned = to_lower(std::string(needle));
    return hay.find(ned) != std::string::npos;
}

inline std::vector<std::string> split(std::string_view value, char delim) {
    std::vector<std::string> out;
    std::string item;
    std::stringstream ss(std::string(value));
    while (std::getline(ss, item, delim)) {
        out.push_back(item);
    }
    return out;
}

inline void replace_all(std::string& s, std::string_view from, std::string_view to) {
    if (from.empty()) {
        return;
    }
    size_t start_pos = 0;
    while ((start_pos = s.find(from.data(), start_pos, from.size())) != std::string::npos) {
        s.replace(start_pos, from.size(), to.data(), to.size());
        start_pos += to.size();
    }
}

inline std::string replace_copy(std::string s, std::string_view from, std::string_view to) {
    replace_all(s, from, to);
    return s;
}

inline std::string join(const std::vector<std::string>& items, std::string_view sep) {
    std::string out;
    for (size_t i = 0; i < items.size(); ++i) {
        out += items[i];
        if (i + 1 < items.size()) {
            out += sep;
        }
    }
    return out;
}

} // namespace silicore::utils
