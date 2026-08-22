#pragma once

#include <string>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <cstdlib>

namespace fs = std::filesystem;

namespace HLMenu {

// ============================================================================
// SVG CONVERTER - CACHED VECTOR ICON RASTERIZATION
// ============================================================================
// Converts standalone SVG files into appropriately-sized PNG icons using
// ImageMagick / rsvg-convert and caches them in `/tmp/hlmenu_cache/`
// to ensure fast and glitch-free rendering.
// ============================================================================

class SvgConverter {
public:
    /**
     * @brief Converts an SVG icon file to PNG if needed, returning the cached PNG path.
     * @param iconPath Path to source icon file.
     * @param size Desired pixel dimension (width & height).
     * @return std::string Path to cached PNG file, or original path if already rasterized.
     */
    static std::string ensurePngIcon(const std::string& iconPath, int size = 64) {
        std::string ext = fs::path(iconPath).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        // If not an SVG, return path as-is
        if (ext != ".svg") {
            return iconPath;
        }

        std::string cacheDir = getCacheDir();
        try {
            fs::create_directories(cacheDir);
        } catch (const std::exception& e) {
            std::cerr << "hlmenu: Failed to create icon cache directory: " << e.what() << "\n";
            return iconPath;
        }

        // Generate collision-resistant cache file name using string hash & target size
        std::string cacheFilename = std::to_string(std::hash<std::string>{}(iconPath)) + 
                                    "_" + std::to_string(size) + ".png";
        std::string cachePath = cacheDir + cacheFilename;

        // Return cached PNG if already generated
        if (fs::exists(cachePath)) {
            return cachePath;
        }

        if (!fs::exists(iconPath)) {
            return iconPath;
        }

        // Convert SVG to transparent PNG using ImageMagick
        std::string cmd = "convert -background none -size " + std::to_string(size) + "x" + std::to_string(size) + 
                          " \"" + iconPath + "\" \"" + cachePath + "\" 2>/dev/null";

        int result = std::system(cmd.c_str());

        if (result == 0 && fs::exists(cachePath)) {
            return cachePath;
        }

        return iconPath;
    }

    /// @brief Checks if a given file path has an .svg extension.
    static bool isSvg(const std::string& path) {
        std::string ext = fs::path(path).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext == ".svg";
    }

    /// @brief Clears the icon rasterization cache.
    static void clearCache() {
        std::string cacheDir = getCacheDir();
        if (fs::exists(cacheDir)) {
            fs::remove_all(cacheDir);
        }
    }

    /// @brief Returns the base cache directory path for hlmenu rasterized icons.
    static std::string getCacheDir() {
        const char* xdgCache = std::getenv("XDG_CACHE_HOME");
        if (xdgCache && std::string(xdgCache).length() > 0) {
            return std::string(xdgCache) + "/hlmenu/icons/";
        }
        return "/tmp/hlmenu_cache/";
    }
};

} // namespace HLMenu
