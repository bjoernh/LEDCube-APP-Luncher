#include "matrixserver_internal.hpp"

#include "cube/detail/hostname_utils.hpp"

#include <unistd.h>   // gethostname (POSIX — macOS + Linux both provide it)
#include <string>

namespace cube::ms {

std::string read_hostname() {
    char buf[256]{};
    if (::gethostname(buf, sizeof(buf) - 1) != 0) {
        return "CUBE";  // fallback
    }
    return cube::detail::normalize_hostname(std::string{buf});
}

int read_battery_percent() {
    // No battery information available on matrixserver hardware.
    // Return -1 to signal "unknown" (same convention as the SDL backend on
    // desktop machines without battery info).
    return -1;
}

} // namespace cube::ms
