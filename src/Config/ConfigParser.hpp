#pragma once

#include "Config.hpp"
#include <string>
#include <optional>
#include <unordered_set>

namespace HLMenu {

    class ConfigParser {
    public:
        static MenuConfig loadConfig(const std::string& customPath = "");
        static std::string getDefaultConfigDir();
        static std::string getDefaultConfigPath();
        static std::string getCustomConfigPath();
        static void ensureDefaultConfigExists();

        // Helper to parse color strings (Hex, RGBA, etc.)
        static std::optional<Hyprtoolkit::CHyprColor> parseColor(const std::string& val);

    private:
        static void parseFile(const std::string& filePath, MenuConfig& config, std::unordered_set<std::string>& visitedFiles);
        static void parseLine(const std::string& line, MenuConfig& config, const std::string& currentFileDir, std::unordered_set<std::string>& visitedFiles);
        static std::string trim(const std::string& str);
    };

}
