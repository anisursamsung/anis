#pragma once

#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/system/Icons.hpp>
#include <hyprutils/memory/SharedPtr.hpp>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <algorithm>

#include "Core/Item.hpp"

namespace fs = std::filesystem;

namespace HLMenu {

// ============================================================================
// ICON RESOLVER - FREEDESKTOP DESKTOP & ICON PARSER
// ============================================================================
// IconResolver parses `.desktop` files, extracts Name, Exec, Icon, NoDisplay,
// and resolves icons using Hyprtoolkit's system icon factory with fallbacks
// to standard XDG pixmap and icon theme paths.
// ============================================================================

class IconResolver {
public:
    /**
     * @brief Parses a .desktop file and resolves its system icon.
     * @param desktopPath Full path to the .desktop file.
     * @param backend Shared pointer to Hyprtoolkit backend.
     * @return Item Valid AppItem, or empty invalid Item if hidden/invalid.
     */
    static Item resolveApp(const std::string& desktopPath,
                           Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IBackend> backend) {
        
        std::string name;
        std::string genericName;
        std::string comment;
        std::string iconName;
        std::string execCmd;
        bool noDisplay = false;
        bool hidden = false;

        std::ifstream file(desktopPath);
        if (file.is_open()) {
            std::string line;
            bool inDesktopEntry = false;

            while (std::getline(file, line)) {
                line.erase(0, line.find_first_not_of(" \t"));
                line.erase(line.find_last_not_of(" \t") + 1);

                if (line.empty() || line[0] == '#') continue;

                if (line[0] == '[') {
                    inDesktopEntry = (line == "[Desktop Entry]");
                    continue;
                }

                if (!inDesktopEntry) continue;

                size_t eqPos = line.find('=');
                if (eqPos == std::string::npos) continue;

                std::string key = line.substr(0, eqPos);
                std::string value = line.substr(eqPos + 1);

                if (key == "Name") {
                    name = value;
                } else if (key == "GenericName") {
                    genericName = value;
                } else if (key == "Comment") {
                    comment = value;
                } else if (key == "Icon") {
                    iconName = value;
                } else if (key == "Exec") {
                    execCmd = value;
                } else if (key == "NoDisplay") {
                    noDisplay = (value == "true");
                } else if (key == "Hidden") {
                    hidden = (value == "true");
                }
            }
        }

        // Filter out entries that should not be visible
        if (name.empty() || execCmd.empty() || noDisplay || hidden) {
            return Item();
        }

        std::string description = !genericName.empty() ? genericName : comment;

        if (!backend) {
            return ItemFactory::makeApp(name, execCmd, desktopPath, nullptr, "", description);
        }

        auto iconFactory = backend->systemIcons();
        if (!iconFactory) {
            return ItemFactory::makeApp(name, execCmd, desktopPath, nullptr, "", description);
        }

        // 1. Try system icon theme lookup
        auto iconDesc = trySystemIcon(iconName, iconFactory);
        if (iconDesc) {
            return ItemFactory::makeApp(name, execCmd, desktopPath, iconDesc, "", description);
        }

        // 2. Try absolute and standard filesystem paths
        std::string iconPath = tryFilesystemPaths(iconName, desktopPath);
        if (!iconPath.empty()) {
            return ItemFactory::makeApp(name, execCmd, desktopPath, nullptr, iconPath, description);
        }

        // 3. Fallback generic executable icon
        iconDesc = tryFallbackIcons(iconFactory);
        if (iconDesc) {
            return ItemFactory::makeApp(name, execCmd, desktopPath, iconDesc, "", description);
        }

        return ItemFactory::makeApp(name, execCmd, desktopPath, nullptr, "", description);
    }

    /**
     * @brief Resolves an icon for a binary executable name (e.g. "htop", "kitty", "bash").
     * @param binaryName Executable command name.
     * @param backend Shared pointer to Hyprtoolkit backend.
     * @return Resolved ISystemIconDescription or fallback icon.
     */
    static Hyprutils::Memory::CSharedPointer<Hyprtoolkit::ISystemIconDescription>
    resolveBinaryIcon(const std::string& binaryName,
                      Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IBackend> backend) {
        if (!backend) return nullptr;
        auto iconFactory = backend->systemIcons();
        if (!iconFactory) return nullptr;

        // 1. Try direct binary name in theme
        auto icon = trySystemIcon(binaryName, iconFactory);
        if (icon) return icon;

        // 2. Fallback to system-run / executable icon
        return tryFallbackIcons(iconFactory);
    }

private:
    static Hyprutils::Memory::CSharedPointer<Hyprtoolkit::ISystemIconDescription>
    trySystemIcon(const std::string& iconName,
                  Hyprutils::Memory::CSharedPointer<Hyprtoolkit::ISystemIconFactory> iconFactory) {
        if (!iconFactory || iconName.empty()) {
            return nullptr;
        }

        auto iconDesc = iconFactory->lookupIcon(iconName);
        if (iconDesc && iconDesc->exists()) {
            return iconDesc;
        }

        return nullptr;
    }

    static std::string tryFilesystemPaths(const std::string& iconName,
                                          const std::string& desktopPath) {
        if (iconName.empty()) {
            return "";
        }

        // Absolute path check
        if (fs::path(iconName).is_absolute()) {
            if (fs::exists(iconName)) {
                return iconName;
            }
        }

        // Check directory relative to .desktop file
        if (!desktopPath.empty()) {
            fs::path desktopDir = fs::path(desktopPath).parent_path();
            static const std::vector<std::string> extensions = {".png", ".svg", ".jpg", ".jpeg", ".xpm", ""};
            for (const auto& ext : extensions) {
                fs::path withExt = desktopDir / (iconName + ext);
                if (fs::exists(withExt)) {
                    return withExt.string();
                }
            }
        }

        // Check common system icon directories
        static const auto iconDirs = []() {
            std::vector<fs::path> dirs = {
                "/usr/share/pixmaps",
                "/usr/share/icons/hicolor/48x48/apps",
                "/usr/share/icons/hicolor/64x64/apps",
                "/usr/share/icons/hicolor/128x128/apps",
                "/usr/share/icons/hicolor/scalable/apps",
                "/usr/share/icons/Adwaita/48x48/apps",
                "/usr/share/icons/Adwaita/scalable/apps",
                "/usr/share/icons/breeze/48x48/apps",
                "/usr/share/icons/breeze/scalable/apps",
            };
            const char* home = std::getenv("HOME");
            if (home && std::strlen(home) > 0) {
                dirs.push_back(fs::path(home) / ".local/share/icons");
                dirs.push_back(fs::path(home) / ".local/share/icons/hicolor/48x48/apps");
            }
            return dirs;
        }();

        static const std::vector<std::string> extensions = {".png", ".svg", ".jpg", ".jpeg", ".xpm", ""};

        for (const auto& dir : iconDirs) {
            if (fs::exists(dir)) {
                for (const auto& ext : extensions) {
                    fs::path checkPath = dir / (iconName + ext);
                    if (fs::exists(checkPath)) {
                        return checkPath.string();
                    }
                }
            }
        }

        return "";
    }

    static Hyprutils::Memory::CSharedPointer<Hyprtoolkit::ISystemIconDescription>
    tryFallbackIcons(Hyprutils::Memory::CSharedPointer<Hyprtoolkit::ISystemIconFactory> iconFactory) {
        if (!iconFactory) {
            return nullptr;
        }

        static const std::vector<std::string> fallbackIcons = {
            "application-x-executable",
            "executable",
            "application-default-icon",
            "unknown",
            "system-run"
        };

        for (const auto& fallback : fallbackIcons) {
            auto iconDesc = iconFactory->lookupIcon(fallback);
            if (iconDesc && iconDesc->exists()) {
                return iconDesc;
            }
        }

        return nullptr;
    }
};

} // namespace HLMenu
