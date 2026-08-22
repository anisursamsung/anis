#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <filesystem>
#include <iostream>
#include <cstdlib>
#include "Core/Item.hpp"
#include "Utils/IconResolver.hpp"

namespace HLMenu {

namespace fs = std::filesystem;

// ============================================================================
// APP PROVIDER - FREEDESKTOP APPLICATION DISCOVERY
// ============================================================================
// AppProvider discovers all installed applications on the system by parsing
// standard XDG/FreeDesktop directories for `.desktop` entry files.
// Supported standard paths:
//   - /usr/share/applications
//   - ~/.local/share/applications
//   - Flatpak export directories (user & system)
//   - Snap desktop directories
// ============================================================================

class AppProvider {
public:
    /**
     * @brief Scans all standard application directories and returns valid desktop apps.
     * @param backend Shared pointer to Hyprtoolkit backend for system icon lookups.
     * @return std::vector<Item> List of discovered, non-hidden application items.
     */
    static std::vector<Item> getApps(Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IBackend> backend) {
        std::vector<Item> items;
        std::unordered_set<std::string> seen;
        std::vector<fs::path> appDirs;

        // 1. User Local Applications & User Flatpaks (highest precedence)
        const char* home = std::getenv("HOME");
        if (home) {
            appDirs.push_back(fs::path(home) / ".local/share/applications");
            appDirs.push_back(fs::path(home) / ".local/share/flatpak/exports/share/applications");
        }

        // 2. System Flatpaks & Snaps
        appDirs.push_back("/var/lib/flatpak/exports/share/applications");
        appDirs.push_back("/var/lib/snapd/desktop/applications");

        // 3. Local & System Applications
        appDirs.push_back("/usr/local/share/applications");
        appDirs.push_back("/usr/share/applications");

        // Scan each directory
        for (const auto& dir : appDirs) {
            if (fs::exists(dir)) {
                try {
                    for (const auto& entry : fs::directory_iterator(dir)) {
                        if (entry.path().extension() == ".desktop") {
                            std::string filename = entry.path().filename().string();
                            if (seen.find(filename) != seen.end()) {
                                continue;
                            }
                            Item app = IconResolver::resolveApp(entry.path().string(), backend);
                            if (app.isValid()) {
                                seen.insert(filename);
                                items.push_back(app);
                            }
                        }
                    }
                } catch (const std::exception& e) {
                    std::cerr << "hlmenu: Error reading application directory " << dir << ": " << e.what() << "\n";
                }
            }
        }

        // Sort applications alphabetically by display name
        std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) {
            return a.displayName() < b.displayName();
        });

        return items;
    }
};

} // namespace HLMenu
