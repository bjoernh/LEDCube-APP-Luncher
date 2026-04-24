#include "sdl_internal.hpp"

#include <cctype>
#include <string>
#include <unistd.h>   // gethostname (POSIX — macOS + Linux both provide it)

namespace cube::sdl {

std::string read_hostname() {
    char buf[256]{};
    if (::gethostname(buf, sizeof(buf) - 1) != 0) {
        return "CUBE";   // reasonable fallback for the PoC
    }
    std::string s{buf};
    if (auto dot = s.find('.'); dot != std::string::npos) s.resize(dot);
    for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

int read_battery_percent() {
    // PoC: the font has no battery glyph and the plan treats this as a
    // static placeholder. Real platforms (iOS, hardware) will replace this.
    return 100;
}

} // namespace cube::sdl
