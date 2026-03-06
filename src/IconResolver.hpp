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

#include "Item.hpp"

namespace fs = std::filesystem;

class IconResolver {
public:
    static Item resolveApp(const std::string& desktopPath,
                           Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IBackend> backend) {
        
        
        // Parse the desktop file
        std::string name;
        std::string iconName;
        std::string execCmd;
        bool noDisplay = false;
        bool hidden = false;
        
        std::ifstream file(desktopPath);
        if (file.is_open()) {
            std::string line;
            bool inDesktopEntry = false;
            
            while (std::getline(file, line)) {
                // Trim whitespace
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
        
        // Skip if invalid - return default Item() which is now invalid (monostate)
        if (name.empty() || execCmd.empty() || noDisplay || hidden) {
            if (name.empty()) std::cout << "Missing Name";
            else if (execCmd.empty()) std::cout << "Missing Exec";
            else if (noDisplay) std::cout << "NoDisplay=true";
            else if (hidden) std::cout << "Hidden=true";
            std::cout << std::endl;
            return Item();  // This creates an invalid item with monostate
        }
        
        
        if (!backend) {
            return ItemFactory::makeApp(name, execCmd, desktopPath, nullptr, "");
        }
        
        auto iconFactory = backend->systemIcons();
        if (!iconFactory) {
            return ItemFactory::makeApp(name, execCmd, desktopPath, nullptr, "");
        }
        
        // Try system icon
        auto iconDesc = trySystemIcon(iconName, iconFactory);
        if (iconDesc) {
            return ItemFactory::makeApp(name, execCmd, desktopPath, iconDesc, "");
        }
        
        // Try filesystem paths
        std::string iconPath = tryFilesystemPaths(iconName, desktopPath);
        if (!iconPath.empty()) {
            return ItemFactory::makeApp(name, execCmd, desktopPath, nullptr, iconPath);
        }
        
        // Try fallback icons
        iconDesc = tryFallbackIcons(iconFactory);
        if (iconDesc) {
            return ItemFactory::makeApp(name, execCmd, desktopPath, iconDesc, "");
        }
        
        return ItemFactory::makeApp(name, execCmd, desktopPath, nullptr, "");
    }

private:
    // ... rest of the helper functions remain exactly the same ...
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
        
        
        // Check absolute path
        if (fs::path(iconName).is_absolute()) {
            if (fs::exists(iconName)) {
                return iconName;
            }
        }
        
        // Check same directory as desktop file
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
        
        // Check common icon directories
        static const std::vector<fs::path> iconDirs = {
            "/usr/share/pixmaps",
            "/usr/share/icons/hicolor/48x48/apps",
            "/usr/share/icons/hicolor/64x64/apps",
            "/usr/share/icons/hicolor/128x128/apps",
            "/usr/share/icons/hicolor/scalable/apps",
            "/usr/share/icons/Adwaita/48x48/apps",
            "/usr/share/icons/Adwaita/scalable/apps",
            "/usr/share/icons/breeze/48x48/apps",
            "/usr/share/icons/breeze/scalable/apps",
            fs::path(std::getenv("HOME")) / ".local/share/icons",
            fs::path(std::getenv("HOME")) / ".local/share/icons/hicolor/48x48/apps",
        };
        
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
