#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

namespace HLMenu {

// ============================================================================
// CLI ARGUMENT PARSER & CONFIGURATION OPTIONS
// ============================================================================

/**
 * @enum MenuMode
 * @brief Specifies the data source mode for hlmenu.
 */
enum class MenuMode {
    APPS,        ///< Scans installed desktop applications (.desktop files)
    RUN,         ///< Discovers and launches system executable binaries from $PATH
    WINDOWS,     ///< Lists active Hyprland open windows / clients
    WORKSPACES,  ///< Lists active Hyprland workspaces
    FILES,       ///< Scans a directory for files (uses generic placeholder icons)
    IMAGES,      ///< Scans a directory for image files (loads image thumbnail preview)
    OPTIONS      ///< Parses a custom delimited string list of options
};

/**
 * @enum ViewStyle
 * @brief Specifies whether items are rendered in a multi-column Grid or vertical List.
 */
enum class ViewStyle {
    GRID,     ///< Multi-column responsive icon grid
    LIST      ///< Single-column vertical list with row highlights
};

/**
 * @enum OutputFormat
 * @brief Output formatting for selected options (string value or numeric index).
 */
enum class OutputFormat {
    STRING,   ///< Print selected item name/action
    INDEX     ///< Print 0-based selected index
};

/**
 * @struct CliOptions
 * @brief Holds command-line flags passed at invocation.
 */
struct CliOptions {
    MenuMode mode = MenuMode::APPS;     ///< Active operating mode
    std::vector<MenuMode> modes;        ///< Combined list of modes (e.g. {WINDOWS, WORKSPACES})
    bool modesExplicitlySet = false;    ///< True if -mode / -modes was passed on CLI
    ViewStyle view = ViewStyle::GRID;   ///< Visual presentation mode
    std::string source = "";            ///< Custom source path or string
    std::string onClick = "";           ///< Custom on-click command template
    std::string title = "";             ///< Custom top bar title override
    std::string configPath = "";        ///< Path to override default config file
    bool viewExplicitlySet = false;     ///< True if -view was provided on CLI
    bool verbose = false;               ///< Enable verbose debug logging (--verbose, -v, HT_DEBUG=1)
    bool passwordMode = false;          ///< True if input characters should be masked (●)
    OutputFormat format = OutputFormat::STRING; ///< Output format (STRING or INDEX)

    std::optional<std::string> prompt;        ///< Search placeholder override (-p, -prompt)
    std::optional<std::string> query;         ///< Initial search query string (-q, -query, -filter)
    std::optional<std::string> sizeStr;       ///< Window dimensions override (-size, -window-size)
    std::optional<std::string> anchorStr;     ///< Window anchor override (-anchor, -pos)
    std::optional<bool> showSearch;           ///< SearchBar visibility override (-no-search)
    std::optional<bool> showTitle;            ///< TitleBar visibility override (-no-title)
    std::optional<bool> showSubtitles;        ///< Subtitles visibility override (-no-subtitles, -subtitles)

    std::unordered_map<MenuMode, std::string> modeSources;   ///< Mode-specific source overrides
    std::unordered_map<MenuMode, std::string> modeOnClicks;  ///< Mode-specific onClick overrides
    std::unordered_map<MenuMode, std::string> modeTitles;    ///< Mode-specific title overrides
    std::unordered_map<MenuMode, ViewStyle> modeViews;       ///< Mode-specific view overrides

    /// @brief Returns the effective source path/string for a given mode.
    std::string getSourceForMode(MenuMode m) const {
        auto it = modeSources.find(m);
        if (it != modeSources.end() && !it->second.empty()) return it->second;
        return source;
    }

    /// @brief Returns the effective onClick command template for a given mode.
    std::string getOnClickForMode(MenuMode m) const {
        auto it = modeOnClicks.find(m);
        if (it != modeOnClicks.end() && !it->second.empty()) return it->second;
        return onClick;
    }

    /// @brief Returns the effective title/tab label for a given mode.
    std::string getTitleForMode(MenuMode m) const {
        auto it = modeTitles.find(m);
        if (it != modeTitles.end() && !it->second.empty()) return it->second;
        if (!title.empty()) return title;
        return "";
    }

    /// @brief Returns the effective view style for a given mode.
    ViewStyle getViewForMode(MenuMode m) const {
        auto it = modeViews.find(m);
        if (it != modeViews.end()) return it->second;
        return view;
    }
};

/**
 * @class ArgumentParser
 * @brief Parses argc/argv into structured CliOptions.
 */
class ArgumentParser {
public:
    /**
     * @brief Parses command line arguments.
     * @param argc Argument count.
     * @param argv Argument vector.
     * @return CliOptions Parsed options struct.
     */
    static CliOptions parse(int argc, char* argv[]);

    /**
     * @brief Prints usage and hotkey instructions to stdout.
     */
    static void printHelp(const char* programName);
    /**
     * @brief Utility to lowercase a string.
     */
    static std::string toLower(std::string str);
};

} // namespace HLMenu
