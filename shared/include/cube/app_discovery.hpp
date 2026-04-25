#pragma once
#include <string>
#include <vector>

namespace cube {

struct DiscoveredApp {
    std::string name;  // filename (e.g. "Snake")
    std::string path;  // absolute path (e.g. "/.../bin/Snake")
};

// Scan CUBE_APP_PATH (or $HOME/APPS, then /home/pi/APPS) for regular files
// with the owner-execute bit set. Returns results sorted alphabetically by name.
// Returns an empty vector if the directory doesn't exist.
std::vector<DiscoveredApp> discover_apps();

} // namespace cube
