#include "cube/detail/hostname_utils.hpp"

namespace cube::detail {

std::string normalize_hostname(std::string s) {
    if (auto p = s.find('.'); p != std::string::npos) s.resize(p);
    for (char& c : s) if (c >= 'a' && c <= 'z') c -= 32;
    return s;
}

} // namespace cube::detail
