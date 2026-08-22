#include "ConfigParser.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <algorithm>
#include <regex>
#include <cstring>

namespace HLMenu {

    std::string ConfigParser::trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos)
            return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

    std::optional<Hyprtoolkit::CHyprColor> ConfigParser::parseColor(const std::string& raw) {
        std::string s = trim(raw);
        if (s.empty())
            return std::nullopt;

        // Remove quotes if present
        if ((s.front() == '"' && s.back() == '"') || (s.front() == '\'' && s.back() == '\'')) {
            s = s.substr(1, s.length() - 2);
            s = trim(s);
        }

        // Handle #RRGGBB or #RRGGBBAA or #RGB or #RGBA
        if (s[0] == '#' || (s.rfind("0x", 0) == 0) || (s.rfind("0X", 0) == 0)) {
            std::string hex = (s[0] == '#') ? s.substr(1) : s.substr(2);
            try {
                if (hex.length() == 6) {
                    uint32_t val = std::stoul(hex, nullptr, 16);
                    float r = ((val >> 16) & 0xFF) / 255.0f;
                    float g = ((val >> 8) & 0xFF) / 255.0f;
                    float b = (val & 0xFF) / 255.0f;
                    return Hyprtoolkit::CHyprColor(r, g, b, 1.0f);
                } else if (hex.length() == 8) {
                    uint32_t val = std::stoul(hex, nullptr, 16);
                    float r = ((val >> 24) & 0xFF) / 255.0f;
                    float g = ((val >> 16) & 0xFF) / 255.0f;
                    float b = ((val >> 8) & 0xFF) / 255.0f;
                    float a = (val & 0xFF) / 255.0f;
                    return Hyprtoolkit::CHyprColor(r, g, b, a);
                } else if (hex.length() == 3) {
                    int r = std::stoi(std::string(2, hex[0]), nullptr, 16);
                    int g = std::stoi(std::string(2, hex[1]), nullptr, 16);
                    int b = std::stoi(std::string(2, hex[2]), nullptr, 16);
                    return Hyprtoolkit::CHyprColor(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
                } else if (hex.length() == 4) {
                    int r = std::stoi(std::string(2, hex[0]), nullptr, 16);
                    int g = std::stoi(std::string(2, hex[1]), nullptr, 16);
                    int b = std::stoi(std::string(2, hex[2]), nullptr, 16);
                    int a = std::stoi(std::string(2, hex[3]), nullptr, 16);
                    return Hyprtoolkit::CHyprColor(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
                }
            } catch (...) {
                return std::nullopt;
            }
        }

        // Handle rgba(r, g, b, a) or rgb(r, g, b)
        std::regex rgbaRegex(R"(rgba?\s*\(\s*([0-9.]+)\s*,\s*([0-9.]+)\s*,\s*([0-9.]+)(?:\s*,\s*([0-9.]+))?\s*\))", std::regex::icase);
        std::smatch match;
        if (std::regex_match(s, match, rgbaRegex)) {
            try {
                float r = std::stof(match[1].str());
                float g = std::stof(match[2].str());
                float b = std::stof(match[3].str());
                float a = match[4].matched ? std::stof(match[4].str()) : 1.0f;
                // If 0-255 scale
                if (r > 1.0f || g > 1.0f || b > 1.0f) {
                    r /= 255.0f;
                    g /= 255.0f;
                    b /= 255.0f;
                }
                return Hyprtoolkit::CHyprColor(r, g, b, a);
            } catch (...) {
                return std::nullopt;
            }
        }

        return std::nullopt;
    }

    std::string ConfigParser::getDefaultConfigDir() {
        const char* xdgConfigHome = std::getenv("XDG_CONFIG_HOME");
        if (xdgConfigHome && std::strlen(xdgConfigHome) > 0) {
            return std::string(xdgConfigHome) + "/hlmenu/";
        }
        const char* home = std::getenv("HOME");
        if (home && std::strlen(home) > 0) {
            return std::string(home) + "/.config/hlmenu/";
        }
        return "./";
    }

    std::string ConfigParser::getDefaultConfigPath() {
        return getDefaultConfigDir() + "hlmenu.conf";
    }

    std::string ConfigParser::getCustomConfigPath() {
        return getDefaultConfigDir() + "custom.conf";
    }

    void ConfigParser::ensureDefaultConfigExists() {
        std::string configDir = getDefaultConfigDir();
        std::filesystem::path dirPath(configDir);

        try {
            std::filesystem::create_directories(dirPath);

            // 1. Ensure hlmenu.conf exists
            std::string mainConf = getDefaultConfigPath();
            if (!std::filesystem::exists(mainConf)) {
                if (std::filesystem::exists("resources/hlmenu.conf")) {
                    std::filesystem::copy_file("resources/hlmenu.conf", mainConf);
                }
            }

            // 2. Ensure custom.conf preset exists
            std::string customConf = getCustomConfigPath();
            if (!std::filesystem::exists(customConf)) {
                if (std::filesystem::exists("resources/custom.conf")) {
                    std::filesystem::copy_file("resources/custom.conf", customConf);
                }
            }
        } catch (...) {}
    }

    void ConfigParser::parseFile(const std::string& filePath, MenuConfig& config, std::unordered_set<std::string>& visitedFiles) {
        std::filesystem::path p(filePath);
        std::string canonical;
        try {
            canonical = std::filesystem::canonical(p).string();
        } catch (...) {
            canonical = p.lexically_normal().string();
        }

        if (visitedFiles.count(canonical)) {
            return; // Cyclic include protection
        }
        visitedFiles.insert(canonical);

        std::ifstream file(canonical);
        if (!file.is_open()) {
            return;
        }

        std::string fileDir = std::filesystem::path(canonical).parent_path().string();
        std::string line;
        while (std::getline(file, line)) {
            parseLine(line, config, fileDir, visitedFiles);
        }
    }

    void ConfigParser::parseLine(const std::string& line, MenuConfig& config, const std::string& currentFileDir, std::unordered_set<std::string>& visitedFiles) {
        std::string clean = trim(line);
        if (clean.empty() || clean[0] == '#' || (clean.rfind("//", 0) == 0))
            return;

        std::string key, val;
        size_t eqPos = clean.find('=');
        if (eqPos != std::string::npos) {
            key = trim(clean.substr(0, eqPos));
            val = trim(clean.substr(eqPos + 1));
        } else {
            // Support space-separated "include <path>" or "source <path>"
            size_t spacePos = clean.find_first_of(" \t");
            if (spacePos != std::string::npos) {
                std::string firstWord = trim(clean.substr(0, spacePos));
                std::string lowerWord = firstWord;
                std::transform(lowerWord.begin(), lowerWord.end(), lowerWord.begin(), ::tolower);
                if (lowerWord == "include" || lowerWord == "source") {
                    key = lowerWord;
                    val = trim(clean.substr(spacePos + 1));
                } else {
                    return;
                }
            } else {
                return;
            }
        }

        // Strip inline comments from value:
        // 1. Double slash '//'
        size_t dslash = val.find("//");
        if (dslash != std::string::npos) {
            val = trim(val.substr(0, dslash));
        }

        // 2. Hash '#' comments (only when preceded by whitespace, preserving hex values like #ffffff)
        if (!val.empty()) {
            size_t startIdx = (val[0] == '#') ? 1 : 0;
            for (size_t i = startIdx; i < val.length(); ++i) {
                if (val[i] == '#' && (val[i - 1] == ' ' || val[i - 1] == '\t')) {
                    val = trim(val.substr(0, i));
                    break;
                }
            }
        }

        std::transform(key.begin(), key.end(), key.begin(), ::tolower);

        if (val.empty())
            return;

        // --- Include / Source Directive ---
        if (key == "include" || key == "source") {
            std::string incPath = val;
            if ((incPath.front() == '"' && incPath.back() == '"') || (incPath.front() == '\'' && incPath.back() == '\'')) {
                incPath = incPath.substr(1, incPath.length() - 2);
                incPath = trim(incPath);
            }

            if (!incPath.empty() && incPath[0] == '~') {
                const char* home = std::getenv("HOME");
                if (home) {
                    incPath = std::string(home) + incPath.substr(1);
                }
            }

            std::filesystem::path resolved;
            if (std::filesystem::exists(incPath)) {
                resolved = incPath;
            } else if (!currentFileDir.empty() && std::filesystem::exists(std::filesystem::path(currentFileDir) / incPath)) {
                resolved = std::filesystem::path(currentFileDir) / incPath;
            } else if (!currentFileDir.empty() && std::filesystem::exists(std::filesystem::path(currentFileDir) / (incPath + ".conf"))) {
                resolved = std::filesystem::path(currentFileDir) / (incPath + ".conf");
            } else if (std::filesystem::exists(getDefaultConfigDir() + incPath)) {
                resolved = getDefaultConfigDir() + incPath;
            } else if (std::filesystem::exists(getDefaultConfigDir() + incPath + ".conf")) {
                resolved = getDefaultConfigDir() + incPath + ".conf";
            } else {
                std::cerr << "hlmenu: Warning: Sourced file '" << val << "' not found.\n";
                return;
            }

            parseFile(resolved.string(), config, visitedFiles);
            return;
        }

        // Colors
        if (key == "background") {
            if (auto col = parseColor(val)) config.background = *col;
        } else if (key == "foreground") {
            if (auto col = parseColor(val)) config.foreground = *col;
        } else if (key == "primary") {
            if (auto col = parseColor(val)) config.primary = *col;
        } else if (key == "on-primary") {
            if (auto col = parseColor(val)) config.onPrimary = *col;
        } else if (key == "secondary") {
            if (auto col = parseColor(val)) config.secondary = *col;
        } else if (key == "on-secondary") {
            if (auto col = parseColor(val)) config.onSecondary = *col;
        } else if (key == "tertiary") {
            if (auto col = parseColor(val)) config.tertiary = *col;
        } else if (key == "on-tertiary") {
            if (auto col = parseColor(val)) config.onTertiary = *col;
        } else if (key == "accent") {
            if (auto col = parseColor(val)) config.accent = *col;
        } else if (key == "on-accent") {
            if (auto col = parseColor(val)) config.onAccent = *col;
        }
        // Fonts
        else if (key == "fontsize") {
            try { config.fontSize = std::stoi(val); } catch (...) {}
        } else if (key == "h1fontsize") {
            try { config.h1FontSize = std::stoi(val); } catch (...) {}
        } else if (key == "h2fontsize") {
            try { config.h2FontSize = std::stoi(val); } catch (...) {}
        } else if (key == "h3fontsize") {
            try { config.h3FontSize = std::stoi(val); } catch (...) {}
        } else if (key == "italic") {
            std::string v = val;
            std::transform(v.begin(), v.end(), v.begin(), ::tolower);
            config.italic = (v == "true" || v == "1" || v == "yes");
        } else if (key == "font-family") {
            config.fontFamily = val;
        } else if (key == "monospace-font") {
            config.monospaceFont = val;
        }
        // Radii & Borders
        else if (key == "corner-radius" || key == "corner-radius-big") {
            try { config.cornerRadiusBig = std::stoi(val); } catch (...) {}
        } else if (key == "corner-radius-small") {
            try { config.cornerRadiusSmall = std::stoi(val); } catch (...) {}
        } else if (key == "border-size" || key == "border-thickness" || key == "border-width") {
            try { config.borderThickness = std::stoi(val); } catch (...) {}
        } else if (key == "item-border-size" || key == "item-border-thickness") {
            try { config.itemBorderSize = std::stoi(val); } catch (...) {}
        } else if (key == "border-color" || key == "border") {
            if (auto col = parseColor(val)) config.borderColor = *col;
        }

        // --- Title Bar (Header / Mode Switcher Tabs) ---
        else if (key == "show-titlebar" || key == "show-title-bar" || key == "show-header") {
            std::string v = val;
            std::transform(v.begin(), v.end(), v.begin(), ::tolower);
            config.showTitleBar = (v == "true" || v == "1" || v == "yes");
        } else if (key == "titlebar-height" || key == "title-bar-height" || key == "header-height" || key == "topbar-height") {
            try {
                config.titleBarHeight = std::stoi(val);
                config.topbarHeight = config.titleBarHeight;
            } catch (...) {}
        } else if (key == "titlebar-gap" || key == "title-bar-gap" || key == "header-gap") {
            try { config.titleBarGap = std::stoi(val); } catch (...) {}
        } else if (key == "show-mode-tabs" || key == "show-tabs" || key == "mode-tabs") {
            std::string v = val;
            std::transform(v.begin(), v.end(), v.begin(), ::tolower);
            config.showModeTabs = (v == "true" || v == "1" || v == "yes");
        } else if (key == "topbar-title-ratio" || key == "searchbar-title-ratio" || key == "topbar-ratio" || key == "searchbar_bar_title_ratio" || key == "searchbar_bar_title_ration") {
            try {
                if (!val.empty() && val.back() == '%') {
                    config.topbarTitleRatio = std::stof(val.substr(0, val.length() - 1)) / 100.0f;
                } else {
                    float v = std::stof(val);
                    if (v > 1.0f) v /= 100.0f;
                    config.topbarTitleRatio = v;
                }
            } catch (...) {}
        } else if (key == "titlebar-title" || key == "topbar-title" || key == "title-text" || key == "menu-title") {
            config.topbarTitle = val;
            config.menuTitle = val;
        } else if (key == "titlebar-title-font-size" || key == "topbar-title-font-size" || key == "title-font-size" || key == "title-size") {
            try { 
                config.topbarTitleFontSize = std::stoi(val);
                config.titleFontSize = config.topbarTitleFontSize;
            } catch (...) {}
        } else if (key == "titlebar-title-font-color" || key == "topbar-title-font-color" || key == "title-font-color" || key == "title-color") {
            if (auto col = parseColor(val)) {
                config.topbarTitleFontColor = *col;
                config.titleFontColor = *col;
            }
        }

        // --- Search Bar ---
        else if (key == "show-searchbar" || key == "show-search-bar" || key == "show-search") {
            std::string v = val;
            std::transform(v.begin(), v.end(), v.begin(), ::tolower);
            config.showSearchBar = (v == "true" || v == "1" || v == "yes");
        } else if (key == "searchbar-height" || key == "search-height") {
            try {
                config.searchBarHeight = std::stoi(val);
                config.searchHeight = config.searchBarHeight;
            } catch (...) {}
        } else if (key == "searchbar-gap" || key == "search-gap") {
            try { config.searchBarGap = std::stoi(val); } catch (...) {}
        } else if (key == "search-background" || key == "searchbar-background") {
            if (auto col = parseColor(val)) config.searchBackground = *col;
        } else if (key == "search-border-color" || key == "searchbar-border-color") {
            if (auto col = parseColor(val)) config.searchBorderColor = *col;
        } else if (key == "search-border-size" || key == "searchbar-border-size") {
            try { config.searchBorderSize = std::stoi(val); } catch (...) {}
        } else if (key == "search-corner-radius" || key == "searchbar-corner-radius") {
            try { config.searchCornerRadius = std::stoi(val); } catch (...) {}
        } else if (key == "search-font-size" || key == "search-size" || key == "searchbar-font-size") {
            try { config.searchFontSize = std::stoi(val); } catch (...) {}
        } else if (key == "search-font-color" || key == "search-color" || key == "searchbar-font-color") {
            if (auto col = parseColor(val)) config.searchFontColor = *col;
        } else if (key == "search-placeholder" || key == "searchbar-placeholder") {
            config.searchPlaceholder = val;
        } else if (key == "search-placeholder-color" || key == "searchbar-placeholder-color") {
            if (auto col = parseColor(val)) config.searchPlaceholderColor = *col;
        } else if (key == "search-width" || key == "searchbar-width") {
            try {
                if (!val.empty() && val.back() == '%') {
                    config.searchWidthPercent = std::stod(val.substr(0, val.length() - 1)) / 100.0;
                    config.searchWidthIsPercent = true;
                } else {
                    double v = std::stod(val);
                    if (v <= 1.0) {
                        config.searchWidthPercent = v;
                        config.searchWidthIsPercent = true;
                    } else {
                        config.searchWidthPercent = v;
                        config.searchWidthIsPercent = false;
                    }
                }
            } catch (...) {}
        } else if (key == "search-height" || key == "searchbar-height") {
            try { config.searchHeight = std::stoi(val); } catch (...) {}
        }

        // --- Subtitles Toggle ---
        else if (key == "show-subtitles" || key == "show-subtitle" || key == "show_subtitles" || key == "subtitles") {
            std::string v = val;
            std::transform(v.begin(), v.end(), v.begin(), ::tolower);
            config.showSubtitles = (v == "true" || v == "1" || v == "yes");
        }

        // --- Grid View & Items ---
        else if (key == "grid-item-width") {
            try { config.gridItemWidth = std::stoi(val); } catch (...) {}
        } else if (key == "grid-item-height") {
            try { config.gridItemHeight = std::stoi(val); } catch (...) {}
        } else if (key == "grid-item-horizontal-gap") {
            try { config.gridItemHorizontalGap = std::stoi(val); } catch (...) {}
        } else if (key == "grid-item-vertical-gap") {
            try { config.gridItemVerticalGap = std::stoi(val); } catch (...) {}
        } else if (key == "grid-item-corner-radius") {
            try { config.gridItemCornerRadius = std::stoi(val); } catch (...) {}
        } else if (key == "grid-item-icon-size") {
            try { config.gridItemIconSize = std::stoi(val); } catch (...) {}
        } else if (key == "grid-item-font-size") {
            try { config.gridItemFontSize = std::stoi(val); } catch (...) {}
        } else if (key == "grid-item-desc-font-size") {
            try { config.gridItemDescFontSize = std::stoi(val); } catch (...) {}
        } else if (key == "grid-item-background" || key == "grid-background") {
            if (auto col = parseColor(val)) config.gridItemBackground = *col;
        } else if (key == "grid-item-border-color" || key == "grid-border-color") {
            if (auto col = parseColor(val)) config.gridItemBorderColor = *col;
        } else if (key == "grid-item-border-size" || key == "grid-border-size") {
            try { config.gridItemBorderSize = std::stoi(val); } catch (...) {}
        } else if (key == "grid-item-active-background") {
            if (auto col = parseColor(val)) config.gridItemActiveBackground = *col;
        } else if (key == "grid-item-active-border-color") {
            if (auto col = parseColor(val)) config.gridItemActiveBorderColor = *col;
        } else if (key == "grid-item-active-border-size") {
            try { config.gridItemActiveBorderSize = std::stoi(val); } catch (...) {}
        } else if (key == "grid-item-padding") {
            try { config.gridItemPadding = std::stoi(val); } catch (...) {}
        } else if (key == "grid-item-font-color") {
            if (auto col = parseColor(val)) config.gridItemFontColor = *col;
        } else if (key == "grid-item-active-font-color") {
            if (auto col = parseColor(val)) config.gridItemActiveFontColor = *col;
        } else if (key == "grid-item-desc-font-color") {
            if (auto col = parseColor(val)) config.gridItemDescFontColor = *col;
        }

        // --- List View & Item ---
        else if (key == "list-item-height") {
            try { config.listItemHeight = std::stoi(val); } catch (...) {}
        } else if (key == "list-items-vertical-gap" || key == "list-item-vertical-gap") {
            try { config.listItemsVerticalGap = std::stoi(val); } catch (...) {}
        } else if (key == "list-item-corner-radius") {
            try { config.listItemCornerRadius = std::stoi(val); } catch (...) {}
        } else if (key == "list-item-padding") {
            try { config.listItemPadding = std::stoi(val); } catch (...) {}
        } else if (key == "list-item-icon-size") {
            try { config.listItemIconSize = std::stoi(val); } catch (...) {}
        } else if (key == "list-item-title-font-size") {
            try { config.listItemTitleFontSize = std::stoi(val); } catch (...) {}
        } else if (key == "list-item-desc-font-size") {
            try { config.listItemDescFontSize = std::stoi(val); } catch (...) {}
        } else if (key == "list-item-background" || key == "list-background") {
            if (auto col = parseColor(val)) config.listItemBackground = *col;
        } else if (key == "list-item-border-color" || key == "list-border-color") {
            if (auto col = parseColor(val)) config.listItemBorderColor = *col;
        } else if (key == "list-item-border-size" || key == "list-border-size") {
            try { config.listItemBorderSize = std::stoi(val); } catch (...) {}
        } else if (key == "list-item-active-background") {
            if (auto col = parseColor(val)) config.listItemActiveBackground = *col;
        } else if (key == "list-item-active-border-color") {
            if (auto col = parseColor(val)) config.listItemActiveBorderColor = *col;
        } else if (key == "list-item-active-border-size") {
            try { config.listItemActiveBorderSize = std::stoi(val); } catch (...) {}
        } else if (key == "list-item-title-font-color" || key == "list-item-font-color") {
            if (auto col = parseColor(val)) config.listItemTitleFontColor = *col;
        } else if (key == "list-item-active-title-font-color" || key == "list-item-active-font-color") {
            if (auto col = parseColor(val)) config.listItemActiveTitleFontColor = *col;
        } else if (key == "list-item-desc-font-color") {
            if (auto col = parseColor(val)) config.listItemDescFontColor = *col;
        }

        // --- Help View ---
        else if (key == "help-header-font-size") {
            try { config.helpHeaderFontSize = std::stoi(val); } catch (...) {}
        } else if (key == "help-header-font-color" || key == "help-header-color") {
            if (auto col = parseColor(val)) config.helpHeaderFontColor = *col;
        } else if (key == "help-text-font-size") {
            try { config.helpTextFontSize = std::stoi(val); } catch (...) {}
        } else if (key == "help-text-font-color" || key == "help-text-color") {
            if (auto col = parseColor(val)) config.helpTextFontColor = *col;
        }

        // Icons
        else if (key == "icon-pack") {
            config.iconPack = val;
        }
        // Window & Geometry
        else if (key == "window-size") {
            size_t xPos = val.find('x');
            if (xPos != std::string::npos) {
                try {
                    double w = std::stod(val.substr(0, xPos));
                    double h = std::stod(val.substr(xPos + 1));
                    config.windowSize = Hyprutils::Math::Vector2D(w, h);
                } catch (...) {}
            }
        } else if (key == "margin-top-left") {
            size_t comma = val.find(',');
            try {
                if (comma != std::string::npos) {
                    config.marginTopLeft = Hyprutils::Math::Vector2D(std::stod(val.substr(0, comma)), std::stod(val.substr(comma + 1)));
                } else {
                    double m = std::stod(val);
                    config.marginTopLeft = Hyprutils::Math::Vector2D(m, m);
                }
            } catch (...) {}
        } else if (key == "margin-bottom-right") {
            size_t comma = val.find(',');
            try {
                if (comma != std::string::npos) {
                    config.marginBottomRight = Hyprutils::Math::Vector2D(std::stod(val.substr(0, comma)), std::stod(val.substr(comma + 1)));
                } else {
                    double m = std::stod(val);
                    config.marginBottomRight = Hyprutils::Math::Vector2D(m, m);
                }
            } catch (...) {}
        } else if (key == "anchor") {
            // Support numbers or bit flags: top=1, bottom=2, left=4, right=8
            uint32_t mask = 0;
            std::stringstream ss(val);
            std::string item;
            while (std::getline(ss, item, '|')) {
                item = trim(item);
                std::transform(item.begin(), item.end(), item.begin(), ::tolower);
                if (item == "top" || item == "1") mask |= 1;
                else if (item == "bottom" || item == "2") mask |= 2;
                else if (item == "left" || item == "4") mask |= 4;
                else if (item == "right" || item == "8") mask |= 8;
                else {
                    try { mask |= std::stoul(item); } catch (...) {}
                }
            }
            config.anchorMask = mask;
        } else if (key == "window-padding") {
            try { config.windowPadding = std::stoi(val); } catch (...) {}
        } else if (key == "mode" || key == "default-mode" || key == "view" || key == "default-view") {
            config.defaultMode = val;
        }
    }

    MenuConfig ConfigParser::loadConfig(const std::string& customPath) {
        MenuConfig config;
        std::string targetPath = customPath;

        if (!targetPath.empty()) {
            // 1. Expand tilde (~/...)
            if (targetPath[0] == '~') {
                const char* home = std::getenv("HOME");
                if (home) {
                    targetPath = std::string(home) + targetPath.substr(1);
                }
            }

            // 2. Check direct path
            if (!std::filesystem::exists(targetPath)) {
                std::string inConfigDir = getDefaultConfigDir() + customPath;

                if (std::filesystem::exists(inConfigDir)) {
                    targetPath = inConfigDir;
                } else if (std::filesystem::exists(inConfigDir + ".conf")) {
                    targetPath = inConfigDir + ".conf";
                } else {
                    std::cerr << "hlmenu: Warning: Custom config '" << customPath 
                              << "' not found. Falling back to default configuration.\n";
                    targetPath = getDefaultConfigPath();
                }
            }
        } else {
            targetPath = getDefaultConfigPath();
        }

        if (!std::filesystem::exists(targetPath)) {
            // Check fallback in resources/hlmenu.conf if running from build/source dir
            if (std::filesystem::exists("resources/hlmenu.conf")) {
                targetPath = "resources/hlmenu.conf";
            } else if (std::filesystem::exists("hlmenu.conf")) {
                targetPath = "hlmenu.conf";
            }
        }

        std::unordered_set<std::string> visitedFiles;
        parseFile(targetPath, config, visitedFiles);

        return config;
    }

}
