#include "cube/app_discovery.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

namespace cube {

std::vector<DiscoveredApp> discover_apps() {
    const char* env = std::getenv("CUBE_APP_PATH");
    std::string dir;
    if (env) {
        dir = env;
    } else {
        const char* home = std::getenv("HOME");
        dir = std::string(home ? home : "/home/pi") + "/APPS";
    }

    std::filesystem::path p{dir};
    if (!std::filesystem::is_directory(p))
        return {};

    std::vector<DiscoveredApp> result;
    for (const auto& entry : std::filesystem::directory_iterator(p)) {
        if (!entry.is_regular_file())
            continue;
        const auto filename = entry.path().filename().string();
        if (filename.empty() || filename[0] == '.')
            continue;
        auto perms = entry.status().permissions();
        if ((perms & std::filesystem::perms::owner_exec) == std::filesystem::perms::none)
            continue;
        result.push_back({filename, entry.path().string()});
    }

    std::sort(result.begin(), result.end(),
              [](const DiscoveredApp& a, const DiscoveredApp& b) {
                  return a.name < b.name;
              });
    return result;
}

} // namespace cube
