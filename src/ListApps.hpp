#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <iostream>
#include "Item.hpp"
#include "IconResolver.hpp"

namespace fs = std::filesystem;

class ListApps {
public:
    static std::vector<Item> getApps(Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IBackend> backend) {
        std::vector<Item> items;
        std::vector<fs::path> appDirs;
        
        appDirs.push_back("/usr/share/applications");
        
        const char* home = std::getenv("HOME");
        if (home) {
            appDirs.push_back(fs::path(home) / ".local/share/applications");
            appDirs.push_back(fs::path(home) / ".local/share/flatpak/exports/share/applications");
        }
        
        appDirs.push_back("/var/lib/flatpak/exports/share/applications");
        appDirs.push_back("/var/lib/snapd/desktop/applications");
        
        for (const auto& dir : appDirs) {
            if (fs::exists(dir)) {
                try {
                    for (const auto& entry : fs::directory_iterator(dir)) {
                        if (entry.path().extension() == ".desktop") {
                            Item app = IconResolver::resolveApp(entry.path().string(), backend);
                            if (app.isValid()) {
                                items.push_back(app);
                            }
                        }
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error reading " << dir << ": " << e.what() << std::endl;
                }
            }
        }
        
        return items;
    }
    
    static std::vector<Item> getAppsFromDir(const std::string& customDir, 
                                           Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IBackend> backend) {
        std::vector<Item> items;
        fs::path dir(customDir);
        
        if (!fs::exists(dir) || !fs::is_directory(dir)) {
            std::cerr << "ListApps: Directory does not exist: " << dir << std::endl;
            return items;
        }
        
        try {
            for (const auto& entry : fs::directory_iterator(dir)) {
                if (entry.path().extension() == ".desktop") {
                    Item app = IconResolver::resolveApp(entry.path().string(), backend);
                    if (app.isValid()) {
                        items.push_back(app);
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error reading " << dir << ": " << e.what() << std::endl;
        }
        
        return items;
    }
};
