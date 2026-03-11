#include "interface/colors.h"
#include "interface/symbols.h"

#include <iostream>

namespace silicore::interface {

void show_explain() {
    using namespace silicore::interface;
    std::cout << c(std::string(symbol("major")) + " What Silicore-C Does", Colors::SKY_DARK) << "\n";
    std::cout << c(std::string(symbol("bullet")) + " Profile: enumerates username presence across platforms using manifest probes.", Colors::GREY) << "\n";
    std::cout << c(std::string(symbol("bullet")) + " Surface: collects DNS/HTTP/robots/security/RDAP surface intel for a domain.", Colors::GREY) << "\n";
    std::cout << c(std::string(symbol("bullet")) + " Fusion: correlates profile + surface results and enriches with plugins.", Colors::GREY) << "\n";
    std::cout << c(std::string(symbol("bullet")) + " Outputs: txt (CLI), json, html. Prompt mode asks for formats and output dir.", Colors::GREY) << "\n";
}

} // namespace silicore::interface

