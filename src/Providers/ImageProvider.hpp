#pragma once

#include <vector>
#include <filesystem>
#include <algorithm>
#include <iostream>
#include <cctype>
#include "Core/Item.hpp"

namespace HLMenu {

namespace fs = std::filesystem;

// ============================================================================
// IMAGE PROVIDER - DIRECTORY SCANNER FOR IMAGES & WALLPAPERS
// ============================================================================
// ImageProvider scans a directory for image files and uses the image path
// directly as the icon source so it renders the image thumbnail/preview.
// ============================================================================

class ImageProvider {
public:
    /**
     * @brief Scans a directory for supported image and media files.
     * @param directory The target folder path.
     * @param onClick Optional command template to execute on click (%f=path, %n=name).
     * @return std::vector<Item> Alphabetically sorted list of image FileItems.
     */
    static std::vector<Item> getImages(const std::string& directory, const std::string& onClick = "") {
        std::vector<Item> items;
        std::string targetDir = directory;
        if (targetDir.empty()) {
            const char* home = std::getenv("HOME");
            std::string picDir = (home && std::strlen(home) > 0) ? (std::string(home) + "/Pictures") : ".";
            targetDir = fs::exists(picDir) ? picDir : ((home && std::strlen(home) > 0) ? home : ".");
        }
        fs::path searchDir(targetDir);

        if (!fs::exists(searchDir) || !fs::is_directory(searchDir)) {
            std::cerr << "hlmenu: ImageProvider directory does not exist: " << searchDir << "\n";
            return items;
        }

        // Supported image and graphic formats
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
                    items.push_back(ItemFactory::makeImage(entry.path(), "", onClick));
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "hlmenu: Error reading image directory " << searchDir << ": " << e.what() << "\n";
        }

        // Sort items alphabetically by filename
        std::sort(items.begin(), items.end(),
            [](const Item& a, const Item& b) {
                return a.displayName() < b.displayName();
            });

        return items;
    }
};

} // namespace HLMenu
