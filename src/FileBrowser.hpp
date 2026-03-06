#pragma once

#include <vector>
#include <filesystem>
#include <algorithm>
#include <iostream>
#include <cctype>
#include "Item.hpp"

namespace fs = std::filesystem;

class FileBrowser {
public:
    static std::vector<Item> getImages(const std::string& directory, const std::string& onClick = "") {
        std::vector<Item> items;
        
        fs::path searchDir(directory);
        
        if (!fs::exists(searchDir) || !fs::is_directory(searchDir)) {
            std::cerr << "FileBrowser: Directory does not exist: " << searchDir << std::endl;
            return items;
        }
        
        std::vector<std::string> imageExtensions = {
            ".png", ".jpg", ".jpeg", ".gif", ".bmp", 
            ".webp", ".tiff", ".tif", ".svg", ".ico"
        };
        
        try {
            for (const auto& entry : fs::directory_iterator(searchDir)) {
                if (!fs::is_regular_file(entry.path())) continue;
                
                auto ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                
                auto it = std::find(imageExtensions.begin(), imageExtensions.end(), ext);
                if (it != imageExtensions.end()) {
                    items.push_back(ItemFactory::makeFile(entry.path(), "", onClick));
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "FileBrowser: Error reading directory: " << e.what() << std::endl;
        }
        
        std::sort(items.begin(), items.end(), 
            [](const Item& a, const Item& b) {
                return a.displayName() < b.displayName();
            });
        
        return items;
    }
};
