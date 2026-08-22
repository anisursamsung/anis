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
// FILE PROVIDER - DIRECTORY SCANNER FOR ALL FILES WITH PLACEHOLDER ICONS
// ============================================================================
// FileProvider scans a given folder for all files and directories.
// Unlike ImageProvider, FileProvider uses standard placeholder icons
// ("text-x-generic" for files and "folder" for directories) instead of
// rendering the file content as an image.
// ============================================================================

class FileProvider {
public:
    /**
     * @brief Scans a directory for all files and subdirectories.
     * @param directory The target folder path.
     * @param onClick Optional command template to execute on click (%f=path, %n=name).
     * @return std::vector<Item> Alphabetically sorted list of FileItems with placeholder icons.
     */
    static std::vector<Item> getFiles(const std::string& directory, const std::string& onClick = "") {
        std::vector<Item> items;
        std::string targetDir = directory;
        if (targetDir.empty()) {
            const char* home = std::getenv("HOME");
            targetDir = (home && std::strlen(home) > 0) ? home : ".";
        }
        fs::path searchDir(targetDir);

        if (!fs::exists(searchDir) || !fs::is_directory(searchDir)) {
            std::cerr << "hlmenu: FileProvider directory does not exist: " << searchDir << "\n";
            return items;
        }

        try {
            for (const auto& entry : fs::directory_iterator(searchDir)) {
                bool isDir = fs::is_directory(entry.path());
                bool isFile = fs::is_regular_file(entry.path());

                if (isDir || isFile) {
                    items.push_back(ItemFactory::makeFile(entry.path(), isDir, onClick));
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "hlmenu: Error reading directory " << searchDir << ": " << e.what() << "\n";
        }

        // Sort: directories first, then files alphabetically
        std::sort(items.begin(), items.end(),
            [](const Item& a, const Item& b) {
                // If one is directory and other is file, directory comes first
                return a.displayName() < b.displayName();
            });

        return items;
    }
};

} // namespace HLMenu
