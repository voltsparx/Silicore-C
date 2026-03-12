#pragma once

#include <string>
#include <vector>

namespace silicore::collect {

struct ExtractedContacts {
    std::vector<std::string> emails;
    std::vector<std::string> phones;
};

std::string extract_bio(const std::string& html);
std::vector<std::string> extract_links(const std::string& html);
ExtractedContacts extract_contacts(const std::string& html);
std::vector<std::string> extract_username_mentions(const std::string& html, const std::string& username);

} // namespace silicore::collect
