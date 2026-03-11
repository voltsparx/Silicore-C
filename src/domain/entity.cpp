#include "domain/entity.h"

#include <openssl/sha.h>
#include <iomanip>
#include <sstream>

namespace silicore::domain {

std::string make_id(std::string_view kind, std::string_view source, std::string_view value) {
    std::string input;
    input.reserve(kind.size() + source.size() + value.size() + 2);
    input.append(kind);
    input.push_back(':');
    input.append(source);
    input.push_back(':');
    input.append(value);

    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(input.data()), input.size(), hash);

    std::ostringstream oss;
    for (unsigned char byte : hash) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    return oss.str();
}

} // namespace silicore::domain

