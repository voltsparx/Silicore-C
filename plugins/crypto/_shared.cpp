#include <iomanip>
#include <sstream>
#include <string>

namespace silicore::crypto {

std::string hex_encode(const std::string& input) {
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (unsigned char c : input) {
        out << std::setw(2) << static_cast<int>(c);
    }
    return out.str();
}

std::string xor_bytes(const std::string& input, const std::string& key) {
    if (key.empty()) {
        return input;
    }
    std::string out = input;
    for (size_t i = 0; i < out.size(); ++i) {
        out[i] = static_cast<char>(static_cast<unsigned char>(out[i]) ^
                                   static_cast<unsigned char>(key[i % key.size()]));
    }
    return out;
}

std::string rot13(const std::string& input) {
    std::string out = input;
    for (char& c : out) {
        if (c >= 'a' && c <= 'z') {
            c = static_cast<char>('a' + (c - 'a' + 13) % 26);
        } else if (c >= 'A' && c <= 'Z') {
            c = static_cast<char>('A' + (c - 'A' + 13) % 26);
        }
    }
    return out;
}

std::string crypto_shared_summary() {
    return "Silicore-C crypto shared helpers available.";
}

} // namespace silicore::crypto
