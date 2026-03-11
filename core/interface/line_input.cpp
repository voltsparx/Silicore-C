#include "core/interface/line_input.h"

#include <iostream>

namespace silicore::interface {

#ifdef SILICORE_USE_LINENOISE
extern "C" {
char* linenoise(const char* prompt);
void linenoiseFree(void* ptr);
void linenoiseHistoryAdd(const char* line);
}
#endif

static thread_local bool g_last_eof = false;

std::string read_line(const std::string& prompt) {
#ifdef SILICORE_USE_LINENOISE
    g_last_eof = false;
    char* line = linenoise(prompt.c_str());
    if (!line) {
        g_last_eof = true;
        return {};
    }
    std::string out(line);
    linenoiseFree(line);
    return out;
#else
    g_last_eof = false;
    std::cout << prompt;
    std::string out;
    if (!std::getline(std::cin, out)) {
        g_last_eof = true;
        return {};
    }
    return out;
#endif
}

void add_history(const std::string& line) {
#ifdef SILICORE_USE_LINENOISE
    if (!line.empty()) {
        linenoiseHistoryAdd(line.c_str());
    }
#else
    (void)line;
#endif
}

bool last_read_eof() {
    return g_last_eof;
}

} // namespace silicore::interface
