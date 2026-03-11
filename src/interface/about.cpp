#include "interface/colors.h"
#include "interface/symbols.h"
#include "foundation/metadata.h"

#include <iostream>

namespace silicore::interface {

void show_about() {
    using namespace silicore::interface;
    std::cout << c(std::string(symbol("major")) + " " + foundation::PROJECT_NAME + " v" + foundation::VERSION, Colors::SKY_DARK) << "\n";
    std::cout << c(std::string(symbol("feature")) + " Theme: " + foundation::VERSION_THEME, Colors::GREY) << "\n";
    std::cout << c(std::string(symbol("feature")) + " Author: " + foundation::AUTHOR, Colors::GREY) << "\n";
    std::cout << c(std::string(symbol("feature")) + " Contact: " + foundation::CONTACT_EMAILS, Colors::GREY) << "\n";
    std::cout << c(std::string(symbol("feature")) + " Project: " + foundation::PROJECT_URL, Colors::GREY) << "\n";
    std::cout << c(std::string(symbol("minor")) + " Purpose: high-signal OSINT profiling and surface analysis.", Colors::CYAN) << "\n";
}

} // namespace silicore::interface

