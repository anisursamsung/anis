#include "ArgumentParser.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <unistd.h>

namespace HLMenu {

std::string ArgumentParser::toLower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

static std::optional<MenuMode> parseSingleMode(const std::string& str) {
    std::string s = ArgumentParser::toLower(str);
    if (s == "apps" || s == "app" || s == "a") return MenuMode::APPS;
    if (s == "run" || s == "bin" || s == "cmd" || s == "exec" || s == "r") return MenuMode::RUN;
    if (s == "windows" || s == "window" || s == "win" || s == "w") return MenuMode::WINDOWS;
    if (s == "workspaces" || s == "workspace" || s == "ws") return MenuMode::WORKSPACES;
    if (s == "files" || s == "file" || s == "f") return MenuMode::FILES;
    if (s == "images" || s == "image" || s == "wallpapers" || s == "wallpaper" || s == "img") return MenuMode::IMAGES;
    if (s == "options" || s == "option" || s == "custom" || s == "o") return MenuMode::OPTIONS;
    return std::nullopt;
}

static std::vector<MenuMode> parseModesList(const std::string& input) {
    std::vector<MenuMode> result;
    std::stringstream ss(input);
    std::string item;
    while (std::getline(ss, item, ',')) {
        std::stringstream ss2(item);
        std::string subItem;
        while (std::getline(ss2, subItem, '+')) {
            std::string trimmed = trim(subItem);
            if (!trimmed.empty()) {
                if (auto m = parseSingleMode(trimmed)) {
                    if (std::find(result.begin(), result.end(), *m) == result.end()) {
                        result.push_back(*m);
                    }
                }
            }
        }
    }
    return result;
}

void ArgumentParser::printHelp(const char* programName) {
    std::cout << "hlmenu - Modern Application Launcher and Menu for Hyprland\n\n"
              << "Usage: " << programName << " [options]\n\n"
              << "General Options:\n"
              << "  -m, --mode <modes>                            Operating mode(s) (apps, run, windows, workspaces, files, images, options)\n"
              << "  -v, --view <grid|list>                        Presentation layout style\n"
              << "  -s, --source <path|string>                    Source directory for files/images, or option string\n"
              << "  -t, --title <text>                            Custom top bar title\n"
              << "  -p, --prompt <text>                           Search bar placeholder text\n"
              << "  -q, --query <text>                            Pre-filled search query\n"
              << "      --onclick <cmd>                           On-click command template (%f=path, %n=name)\n"
              << "      --size <WxH>                              Window dimensions override (e.g. 450x260)\n"
              << "      --anchor <pos>                            Window position (center, top, bottom, etc.)\n"
              << "      --password                                Mask search input with dots (●)\n"
              << "  -i, --format <string|index>                   Output selected string or numeric index\n"
              << "      --no-search                               Hide the search bar\n"
              << "      --no-title                                Hide the title bar\n"
              << "      --no-subtitles                            Hide item subtitles\n"
              << "      --dmenu                                   Drop-in dmenu/rofi pipe mode\n"
              << "  -c, --config <path>                           Custom configuration file path\n"
              << "      --verbose                                 Enable verbose debug output\n"
              << "  -h, --help                                    Display this help information\n\n"
              << "Mode-Specific Overrides (for multi-mode setups):\n"
              << "      --source-<mode> <path>                    Override source for a specific mode\n"
              << "      --title-<mode> <text>                     Override tab label for a specific mode\n"
              << "      --onclick-<mode> <cmd>                    Override on-click command for a specific mode\n"
              << "      --view-<mode> <grid|list>                 Override layout view for a specific mode\n\n"
              << "Navigation & Hotkeys:\n"
              << "  Arrow Keys                                    Navigate items\n"
              << "  Enter                                         Launch / activate selected item\n"
              << "  Escape                                        Close menu (Exit code 1)\n"
              << "  Shift+Left / Shift+Right                      Cycle between combined mode tabs\n"
              << "  Ctrl+Escape                                   Toggle Grid / List view in-place\n";
}

CliOptions ArgumentParser::parse(int argc, char* argv[]) {
    CliOptions options;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        std::string lowerArg = toLower(arg);

        if (lowerArg == "-h" || lowerArg == "--help") {
            printHelp(argv[0]);
            std::exit(0);
        } else if (lowerArg == "-m" || lowerArg == "--mode" || lowerArg == "--modes") {
            if (i + 1 < argc) {
                std::string modeStr = argv[++i];
                auto parsedModes = parseModesList(modeStr);
                if (parsedModes.empty()) {
                    std::cerr << "hlmenu: Invalid mode(s) '" << modeStr << "'\n";
                    std::exit(1);
                }
                options.modes = parsedModes;
                options.mode = parsedModes.front();
                options.modesExplicitlySet = true;
            }
        } else if (lowerArg == "-v" || lowerArg == "--view") {
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                std::string viewStr = toLower(argv[++i]);
                if (viewStr == "list") {
                    options.view = ViewStyle::LIST;
                    options.viewExplicitlySet = true;
                } else if (viewStr == "grid") {
                    options.view = ViewStyle::GRID;
                    options.viewExplicitlySet = true;
                } else {
                    std::cerr << "hlmenu: Invalid view style '" << viewStr << "'\n";
                    std::exit(1);
                }
            } else {
                std::cerr << "hlmenu: Option '--view' / '-v' requires an argument <grid|list>\n";
                std::exit(1);
            }
        } else if (lowerArg == "--view-windows") {
            if (i + 1 < argc) {
                std::string v = toLower(argv[++i]);
                options.modeViews[MenuMode::WINDOWS] = (v == "list") ? ViewStyle::LIST : ViewStyle::GRID;
            }
        } else if (lowerArg == "--view-workspaces") {
            if (i + 1 < argc) {
                std::string v = toLower(argv[++i]);
                options.modeViews[MenuMode::WORKSPACES] = (v == "list") ? ViewStyle::LIST : ViewStyle::GRID;
            }
        } else if (lowerArg == "--view-files") {
            if (i + 1 < argc) {
                std::string v = toLower(argv[++i]);
                options.modeViews[MenuMode::FILES] = (v == "list") ? ViewStyle::LIST : ViewStyle::GRID;
            }
        } else if (lowerArg == "--view-images") {
            if (i + 1 < argc) {
                std::string v = toLower(argv[++i]);
                options.modeViews[MenuMode::IMAGES] = (v == "list") ? ViewStyle::LIST : ViewStyle::GRID;
            }
        } else if (lowerArg == "--view-apps") {
            if (i + 1 < argc) {
                std::string v = toLower(argv[++i]);
                options.modeViews[MenuMode::APPS] = (v == "list") ? ViewStyle::LIST : ViewStyle::GRID;
            }
        } else if (lowerArg == "--view-run") {
            if (i + 1 < argc) {
                std::string v = toLower(argv[++i]);
                options.modeViews[MenuMode::RUN] = (v == "list") ? ViewStyle::LIST : ViewStyle::GRID;
            }
        } else if (lowerArg == "--view-options") {
            if (i + 1 < argc) {
                std::string v = toLower(argv[++i]);
                options.modeViews[MenuMode::OPTIONS] = (v == "list") ? ViewStyle::LIST : ViewStyle::GRID;
            }
        } else if (lowerArg == "--password" || lowerArg == "--mask" || lowerArg == "--hidden") {
            options.passwordMode = true;
        } else if (lowerArg == "--format") {
            if (i + 1 < argc) {
                std::string f = toLower(argv[++i]);
                if (f == "i" || f == "index" || f == "d" || f == "idx") {
                    options.format = OutputFormat::INDEX;
                } else {
                    options.format = OutputFormat::STRING;
                }
            }
        } else if (lowerArg == "-i") {
            options.format = OutputFormat::INDEX;
        } else if (lowerArg == "-s" || lowerArg == "--source") {
            if (i + 1 < argc) {
                options.source = argv[++i];
            }
        } else if (lowerArg == "--source-files") {
            if (i + 1 < argc) {
                options.modeSources[MenuMode::FILES] = argv[++i];
            }
        } else if (lowerArg == "--source-images") {
            if (i + 1 < argc) {
                options.modeSources[MenuMode::IMAGES] = argv[++i];
            }
        } else if (lowerArg == "--source-options") {
            if (i + 1 < argc) {
                options.modeSources[MenuMode::OPTIONS] = argv[++i];
            }
        } else if (lowerArg == "--source-apps") {
            if (i + 1 < argc) {
                options.modeSources[MenuMode::APPS] = argv[++i];
            }
        } else if (lowerArg == "--source-run") {
            if (i + 1 < argc) {
                options.modeSources[MenuMode::RUN] = argv[++i];
            }
        } else if (lowerArg == "--source-windows") {
            if (i + 1 < argc) {
                options.modeSources[MenuMode::WINDOWS] = argv[++i];
            }
        } else if (lowerArg == "--source-workspaces") {
            if (i + 1 < argc) {
                options.modeSources[MenuMode::WORKSPACES] = argv[++i];
            }
        } else if (lowerArg == "-t" || lowerArg == "--title") {
            if (i + 1 < argc) {
                options.title = argv[++i];
            }
        } else if (lowerArg == "--title-options") {
            if (i + 1 < argc) {
                options.modeTitles[MenuMode::OPTIONS] = argv[++i];
            }
        } else if (lowerArg == "--title-files") {
            if (i + 1 < argc) {
                options.modeTitles[MenuMode::FILES] = argv[++i];
            }
        } else if (lowerArg == "--title-images") {
            if (i + 1 < argc) {
                options.modeTitles[MenuMode::IMAGES] = argv[++i];
            }
        } else if (lowerArg == "--title-apps") {
            if (i + 1 < argc) {
                options.modeTitles[MenuMode::APPS] = argv[++i];
            }
        } else if (lowerArg == "--title-run") {
            if (i + 1 < argc) {
                options.modeTitles[MenuMode::RUN] = argv[++i];
            }
        } else if (lowerArg == "--title-windows") {
            if (i + 1 < argc) {
                options.modeTitles[MenuMode::WINDOWS] = argv[++i];
            }
        } else if (lowerArg == "--title-workspaces") {
            if (i + 1 < argc) {
                options.modeTitles[MenuMode::WORKSPACES] = argv[++i];
            }
        } else if (lowerArg == "--onclick" || lowerArg == "--on-click") {
            if (i + 1 < argc) {
                options.onClick = argv[++i];
            }
        } else if (lowerArg == "--onclick-files" || lowerArg == "--on-click-files") {
            if (i + 1 < argc) {
                options.modeOnClicks[MenuMode::FILES] = argv[++i];
            }
        } else if (lowerArg == "--onclick-images" || lowerArg == "--on-click-images") {
            if (i + 1 < argc) {
                options.modeOnClicks[MenuMode::IMAGES] = argv[++i];
            }
        } else if (lowerArg == "--onclick-options" || lowerArg == "--on-click-options") {
            if (i + 1 < argc) {
                options.modeOnClicks[MenuMode::OPTIONS] = argv[++i];
            }
        } else if (lowerArg == "--onclick-apps" || lowerArg == "--on-click-apps") {
            if (i + 1 < argc) {
                options.modeOnClicks[MenuMode::APPS] = argv[++i];
            }
        } else if (lowerArg == "--onclick-run" || lowerArg == "--on-click-run") {
            if (i + 1 < argc) {
                options.modeOnClicks[MenuMode::RUN] = argv[++i];
            }
        } else if (lowerArg == "--onclick-windows" || lowerArg == "--on-click-windows") {
            if (i + 1 < argc) {
                options.modeOnClicks[MenuMode::WINDOWS] = argv[++i];
            }
        } else if (lowerArg == "--onclick-workspaces" || lowerArg == "--on-click-workspaces") {
            if (i + 1 < argc) {
                options.modeOnClicks[MenuMode::WORKSPACES] = argv[++i];
            }
        } else if (lowerArg == "-p" || lowerArg == "--prompt" || lowerArg == "--placeholder") {
            if (i + 1 < argc) {
                options.prompt = argv[++i];
            }
        } else if (lowerArg == "-q" || lowerArg == "--query" || lowerArg == "--filter") {
            if (i + 1 < argc) {
                options.query = argv[++i];
            }
        } else if (lowerArg == "--size" || lowerArg == "--window-size") {
            if (i + 1 < argc) {
                options.sizeStr = argv[++i];
            }
        } else if (lowerArg == "--anchor" || lowerArg == "--pos" || lowerArg == "--position") {
            if (i + 1 < argc) {
                options.anchorStr = argv[++i];
            }
        } else if (lowerArg == "--no-search" || lowerArg == "--no-searchbar") {
            options.showSearch = false;
        } else if (lowerArg == "--search" || lowerArg == "--searchbar") {
            if (i + 1 < argc && (std::string(argv[i + 1]) == "false" || std::string(argv[i + 1]) == "0")) {
                options.showSearch = false;
                ++i;
            } else if (i + 1 < argc && (std::string(argv[i + 1]) == "true" || std::string(argv[i + 1]) == "1")) {
                options.showSearch = true;
                ++i;
            } else {
                options.showSearch = true;
            }
        } else if (lowerArg == "--no-title" || lowerArg == "--no-titlebar") {
            options.showTitle = false;
        } else if (lowerArg == "--titlebar") {
            if (i + 1 < argc && (std::string(argv[i + 1]) == "false" || std::string(argv[i + 1]) == "0")) {
                options.showTitle = false;
                ++i;
            } else if (i + 1 < argc && (std::string(argv[i + 1]) == "true" || std::string(argv[i + 1]) == "1")) {
                options.showTitle = true;
                ++i;
            } else {
                options.showTitle = true;
            }
        } else if (lowerArg == "--no-subtitles" || lowerArg == "--no-subtitle") {
            options.showSubtitles = false;
        } else if (lowerArg == "--subtitles" || lowerArg == "--subtitle") {
            if (i + 1 < argc && (std::string(argv[i + 1]) == "false" || std::string(argv[i + 1]) == "0")) {
                options.showSubtitles = false;
                ++i;
            } else if (i + 1 < argc && (std::string(argv[i + 1]) == "true" || std::string(argv[i + 1]) == "1")) {
                options.showSubtitles = true;
                ++i;
            } else {
                options.showSubtitles = true;
            }
        } else if (lowerArg == "--dmenu") {
            options.mode = MenuMode::OPTIONS;
            options.modes = {MenuMode::OPTIONS};
            options.modesExplicitlySet = true;
        } else if (lowerArg == "-c" || lowerArg == "--config" || lowerArg == "--conf") {
            if (i + 1 < argc) {
                options.configPath = argv[++i];
            }
        } else if (lowerArg == "--verbose" || lowerArg == "--debug") {
            options.verbose = true;
        } else if (arg[0] == '-') {
            std::cerr << "hlmenu: Unknown or invalid option '" << arg << "'\n";
            if (arg.length() > 2 && arg[0] == '-' && arg[1] != '-') {
                std::cerr << "hint: Long options must use double dashes (e.g. '--" << arg.substr(1) << "')\n";
            }
            printHelp(argv[0]);
            std::exit(1);
        }
    }

    // Check if input is piped through stdin (e.g. echo "A\nB" | hlmenu or -source -)
    if (!isatty(STDIN_FILENO) || options.source == "-") {
        std::string pipedInput;
        std::string line;
        while (std::getline(std::cin, line)) {
            pipedInput += line + "\n";
        }
        if (!pipedInput.empty()) {
            options.source = pipedInput;
            if (!options.modesExplicitlySet) {
                options.mode = MenuMode::OPTIONS;
                options.modes = {MenuMode::OPTIONS};
            }
        }
    }

    // Check HT_DEBUG, DEBUG, or HLMENU_DEBUG environment variables
    const char* envDebug = std::getenv("HT_DEBUG");
    if (!envDebug) envDebug = std::getenv("DEBUG");
    if (!envDebug) envDebug = std::getenv("HLMENU_DEBUG");
    if (envDebug && (std::string(envDebug) == "1" || std::string(envDebug) == "true")) {
        options.verbose = true;
    }

    if (options.mode == MenuMode::OPTIONS && options.source.empty() && !options.passwordMode) {
        std::cerr << "hlmenu: Error -mode options requires -source <item list> or piped standard input\n";
        std::exit(1);
    }

    return options;
}

} // namespace HLMenu
