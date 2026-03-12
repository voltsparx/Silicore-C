#include "collect/extractor.h"

#include "utils/strings.h"

#include <algorithm>
#include <regex>
#include <set>

namespace silicore::collect {

namespace {

const std::regex kEmailRegex(R"(\b[a-zA-Z0-9_.+-]+@[a-zA-Z0-9-]+\.[a-zA-Z0-9-.]+\b)");
const std::regex kPhoneRegex(R"((?:\+?\d[\d\s().-]{6,}\d))");
const std::regex kScriptStyleRegex(R"(<(script|style)\b[^>]*>.*?</\1>)", std::regex::icase | std::regex::optimize | std::regex::dotall);
const std::regex kTagRegex(R"(<[^>]+>)", std::regex::optimize | std::regex::icase);

const std::vector<std::regex> kMetaDescriptionPatterns = {
    std::regex(R"(<meta[^>]*name=['"]description['"][^>]*content=['"](.*?)['"][^>]*>)", std::regex::icase | std::regex::dotall),
    std::regex(R"(<meta[^>]*content=['"](.*?)['"][^>]*name=['"]description['"][^>]*>)", std::regex::icase | std::regex::dotall),
    std::regex(R"(<meta[^>]*property=['"]og:description['"][^>]*content=['"](.*?)['"][^>]*>)", std::regex::icase | std::regex::dotall),
    std::regex(R"(<meta[^>]*content=['"](.*?)['"][^>]*property=['"]og:description['"][^>]*>)", std::regex::icase | std::regex::dotall),
    std::regex(R"(<meta[^>]*name=['"]twitter:description['"][^>]*content=['"](.*?)['"][^>]*>)", std::regex::icase | std::regex::dotall),
    std::regex(R"(<meta[^>]*content=['"](.*?)['"][^>]*name=['"]twitter:description['"][^>]*>)", std::regex::icase | std::regex::dotall),
};

std::string unescape_html(std::string text) {
    utils::replace_all(text, "&amp;", "&");
    utils::replace_all(text, "&lt;", "<");
    utils::replace_all(text, "&gt;", ">");
    utils::replace_all(text, "&quot;", "\"");
    utils::replace_all(text, "&#39;", "'");
    return text;
}

std::string clean_text(const std::string& text) {
    if (text.empty()) {
        return "";
    }
    std::string normalized = unescape_html(text);
    std::string output;
    output.reserve(normalized.size());
    bool in_space = false;
    for (char ch : normalized) {
        if (std::isspace(static_cast<unsigned char>(ch))) {
            if (!in_space) {
                output.push_back(' ');
                in_space = true;
            }
            continue;
        }
        in_space = false;
        output.push_back(ch);
    }
    output = utils::trim(output);
    return output;
}

std::string strip_scripts_and_styles(const std::string& text) {
    try {
        return std::regex_replace(text, kScriptStyleRegex, " ");
    } catch (const std::regex_error&) {
        return text;
    }
}

std::string strip_tags(const std::string& text) {
    try {
        return std::regex_replace(text, kTagRegex, " ");
    } catch (const std::regex_error&) {
        return text;
    }
}

std::string html_to_text(const std::string& html) {
    auto without_scripts = strip_scripts_and_styles(html);
    return clean_text(strip_tags(without_scripts));
}

} // namespace

std::string extract_bio(const std::string& html) {
    if (html.empty()) {
        return "";
    }
    for (const auto& pattern : kMetaDescriptionPatterns) {
        std::smatch match;
        if (std::regex_search(html, match, pattern) && match.size() > 1) {
            auto cleaned = clean_text(match[1].str());
            if (!cleaned.empty()) {
                return cleaned;
            }
        }
    }
    try {
        std::regex paragraph(R"(<p[^>]*>(.*?)</p>)", std::regex::icase | std::regex::dotall);
        std::smatch match;
        if (std::regex_search(html, match, paragraph) && match.size() > 1) {
            auto cleaned = clean_text(strip_tags(match[1].str()));
            return cleaned;
        }
    } catch (const std::regex_error&) {
        return "";
    }
    return "";
}

std::vector<std::string> extract_links(const std::string& html) {
    std::vector<std::string> links;
    if (html.empty()) {
        return links;
    }
    try {
        std::regex link_re(R"(href\s*=\s*['"](https?://[^'\"#]+)['"])", std::regex::icase);
        std::set<std::string> seen;
        for (std::sregex_iterator it(html.begin(), html.end(), link_re), end; it != end; ++it) {
            if (it->size() < 2) {
                continue;
            }
            std::string link = (*it)[1].str();
            link = clean_text(link);
            if (link.empty()) {
                continue;
            }
            if (seen.insert(link).second) {
                links.push_back(link);
            }
        }
    } catch (const std::regex_error&) {
        return {};
    }
    return links;
}

ExtractedContacts extract_contacts(const std::string& html) {
    ExtractedContacts contacts;
    std::string text = html_to_text(html);
    if (text.empty()) {
        return contacts;
    }

    std::set<std::string> emails;
    try {
        for (std::sregex_iterator it(text.begin(), text.end(), kEmailRegex), end; it != end; ++it) {
            std::string value = utils::to_lower(it->str());
            if (!value.empty()) {
                emails.insert(value);
            }
        }
    } catch (const std::regex_error&) {
        // ignore
    }
    contacts.emails.assign(emails.begin(), emails.end());

    std::set<std::string> phones;
    try {
        for (std::sregex_iterator it(text.begin(), text.end(), kPhoneRegex), end; it != end; ++it) {
            std::string value = clean_text(it->str());
            std::string digits;
            for (char ch : value) {
                if (std::isdigit(static_cast<unsigned char>(ch))) {
                    digits.push_back(ch);
                }
            }
            if (digits.size() >= 8 && digits.size() <= 15) {
                phones.insert(value);
            }
        }
    } catch (const std::regex_error&) {
        // ignore
    }
    contacts.phones.assign(phones.begin(), phones.end());

    return contacts;
}

std::vector<std::string> extract_username_mentions(const std::string& html, const std::string& username) {
    std::vector<std::string> mentions;
    if (username.empty()) {
        return mentions;
    }
    std::string text = html_to_text(html);
    if (text.empty()) {
        return mentions;
    }
    std::set<std::string> seen;
    std::vector<std::string> patterns = {
        "\\b" + username + "\\b",
        "@" + username,
        "/" + username,
    };
    for (const auto& pattern : patterns) {
        try {
            std::regex re(pattern, std::regex::icase);
            for (std::sregex_iterator it(text.begin(), text.end(), re), end; it != end; ++it) {
                auto value = it->str();
                if (seen.insert(value).second) {
                    mentions.push_back(value);
                }
            }
        } catch (const std::regex_error&) {
            continue;
        }
    }
    return mentions;
}

} // namespace silicore::collect
