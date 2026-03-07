#pragma once

#include <string>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <cstdlib>

namespace fs = std::filesystem;

class SvgConverter {
public:
    /**
     * Convert SVG to PNG if needed, returns PNG path or original path
     * 
     * @param iconPath Path to the icon file
     * @param size Desired size for conversion (default 64)
     * @return Path to PNG file (cached) or original path if not SVG
     */
    static std::string ensurePngIcon(const std::string& iconPath, int size = 64) {
        // Check if it's an SVG file
        std::string ext = fs::path(iconPath).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        if (ext != ".svg") {
            return iconPath;  // Not an SVG, return original
        }
        
        std::cout << "[SvgConverter] Converting SVG: " << iconPath << std::endl;
        
        // Create cache directory
        std::string cacheDir = "/tmp/anis_cache/";
        try {
            fs::create_directories(cacheDir);
        } catch (const std::exception& e) {
            std::cerr << "[SvgConverter] Failed to create cache directory: " << e.what() << std::endl;
            return iconPath;
        }
        
        // Create cache filename from the original path (using hash to avoid collisions)
        std::string cacheFilename = std::to_string(std::hash<std::string>{}(iconPath)) + ".png";
        std::string cachePath = cacheDir + cacheFilename;
        
        // Return cached PNG if it exists
        if (fs::exists(cachePath)) {
            std::cout << "[SvgConverter] Using cached PNG: " << cachePath << std::endl;
            return cachePath;
        }
        
        // Check if source file exists
        if (!fs::exists(iconPath)) {
            std::cerr << "[SvgConverter] SVG file does not exist: " << iconPath << std::endl;
            return iconPath;
        }
        
        // Convert SVG to PNG using ImageMagick
        std::string cmd = "convert -background none -size " + std::to_string(size) + "x" + std::to_string(size) + 
                          " \"" + iconPath + "\" \"" + cachePath + "\" 2>/dev/null";
        
        int result = std::system(cmd.c_str());
        
        if (result == 0 && fs::exists(cachePath)) {
            std::cout << "[SvgConverter] Successfully converted SVG to PNG: " << cachePath << std::endl;
            return cachePath;
        } else {
            std::cerr << "[SvgConverter] Failed to convert SVG: " << iconPath << std::endl;
            return iconPath;
        }
    }
    
    /**
     * Check if a file is an SVG
     */
    static bool isSvg(const std::string& path) {
        std::string ext = fs::path(path).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext == ".svg";
    }
    
    /**
     * Clear the SVG cache directory
     */
    static void clearCache() {
        std::string cacheDir = "/tmp/anis_cache/";
        if (fs::exists(cacheDir)) {
            fs::remove_all(cacheDir);
            std::cout << "[SvgConverter] Cache cleared" << std::endl;
        }
    }
    
    /**
     * Get cache directory path
     */
    static std::string getCacheDir() {
        return "/tmp/anis_cache/";
    }
};
