#include "Cli/ArgumentParser.hpp"
#include "Config/ConfigParser.hpp"
#include "Core/ItemList.hpp"
#include "Providers/AppProvider.hpp"
#include "Providers/RunProvider.hpp"
#include "Providers/FileProvider.hpp"
#include "Providers/ImageProvider.hpp"
#include "Providers/WindowProvider.hpp"
#include "Providers/WorkspaceProvider.hpp"
#include "Providers/CustomOptionsParser.hpp"
#include "Views/Grid/GridView.hpp"
#include "Views/List/ListView.hpp"
#include "Components/TitleBar.hpp"
#include "Components/SearchBar.hpp"

#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <xkbcommon/xkbcommon-keysyms.h>

#include <iostream>
#include <atomic>
#include <memory>
#include <algorithm>
#include <vector>
#include <fcntl.h>
#include <unistd.h>

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

namespace HLMenu {

// ============================================================================
// ITEM DISPATCH HELPER
// ============================================================================
/**
 * @brief Loads items based on CLI mode (APPS, RUN, WINDOWS, WORKSPACES, FILES, IMAGES, or OPTIONS).
 */
std::vector<Item> loadItems(const CliOptions& cli, CSharedPointer<IBackend> backend) {
    switch (cli.mode) {
        case MenuMode::APPS:
            return AppProvider::getApps(backend);

        case MenuMode::RUN:
            return RunProvider::getExecutables(backend);

        case MenuMode::WINDOWS:
            return WindowProvider::getWindows(backend);

        case MenuMode::WORKSPACES:
            return WorkspaceProvider::getWorkspaces(backend);

        case MenuMode::FILES:
            return FileProvider::getFiles(cli.getSourceForMode(MenuMode::FILES), cli.getOnClickForMode(MenuMode::FILES));

        case MenuMode::IMAGES:
            return ImageProvider::getImages(cli.getSourceForMode(MenuMode::IMAGES), cli.getOnClickForMode(MenuMode::IMAGES));

        case MenuMode::OPTIONS:
            return CustomOptionsParser::parseOptions(cli.getSourceForMode(MenuMode::OPTIONS), cli.getOnClickForMode(MenuMode::OPTIONS));

        default:
            return {};
    }
}

} // namespace HLMenu

// ============================================================================
// MAIN APPLICATION ENTRY POINT
// ============================================================================
int main(int argc, char* argv[]) {
    // ------------------------------------------------------------------------
    // STEP 1: Parse Command-Line Arguments (Default Silent Execution)
    // ------------------------------------------------------------------------
    HLMenu::CliOptions cli = HLMenu::ArgumentParser::parse(argc, argv);

    // Silent by default: suppress toolkit / hyprlang stderr diagnostic spam unless --verbose / -v / HT_DEBUG=1
    if (!cli.verbose) {
        int nullFd = open("/dev/null", O_WRONLY);
        if (nullFd >= 0) {
            dup2(nullFd, STDERR_FILENO);
            close(nullFd);
        }
    }

    // ------------------------------------------------------------------------
    // STEP 2: Load Configuration (~/.config/hlmenu/hlmenu.conf)
    // ------------------------------------------------------------------------
    HLMenu::ConfigParser::ensureDefaultConfigExists();
    HLMenu::MenuConfig config = HLMenu::ConfigParser::loadConfig(cli.configPath);

    // CLI overrides for title, prompt, subtitles, visibility, window size, anchor
    if (!cli.title.empty()) {
        config.topbarTitle = cli.title;
    }
    if (cli.prompt) {
        config.searchPlaceholder = *cli.prompt;
    }
    if (cli.showSubtitles) {
        config.showSubtitles = *cli.showSubtitles;
    }
    if (cli.showSearch) {
        config.showSearchBar = *cli.showSearch;
    }
    if (cli.showTitle) {
        config.showTitleBar = *cli.showTitle;
    }
    if (cli.sizeStr) {
        std::string s = *cli.sizeStr;
        size_t xPos = s.find('x');
        if (xPos == std::string::npos) xPos = s.find('X');
        if (xPos != std::string::npos) {
            try {
                int w = std::stoi(s.substr(0, xPos));
                int h = std::stoi(s.substr(xPos + 1));
                if (w > 50 && h > 50) {
                    config.windowSize = Hyprutils::Math::Vector2D(w, h);
                }
            } catch (...) {}
        }
    }
    if (cli.anchorStr) {
        std::string a = HLMenu::ArgumentParser::toLower(*cli.anchorStr);
        if (a == "top" || a == "t") config.anchorMask = 1;
        else if (a == "bottom" || a == "b") config.anchorMask = 2;
        else if (a == "left" || a == "l") config.anchorMask = 4;
        else if (a == "right" || a == "r") config.anchorMask = 8;
        else if (a == "top-left" || a == "tl") config.anchorMask = 1 | 4;
        else if (a == "top-right" || a == "tr") config.anchorMask = 1 | 8;
        else if (a == "bottom-left" || a == "bl") config.anchorMask = 2 | 4;
        else if (a == "bottom-right" || a == "br") config.anchorMask = 2 | 8;
        else if (a == "center" || a == "c") config.anchorMask = 0;
    }

    // ------------------------------------------------------------------------
    // STEP 3: Determine Initial View Mode (Grid vs List)
    // ------------------------------------------------------------------------
    HLMenu::ViewStyle activeView = cli.getViewForMode(cli.mode);
    if (!cli.viewExplicitlySet && config.defaultMode == "list" && cli.modeViews.find(cli.mode) == cli.modeViews.end()) {
        activeView = HLMenu::ViewStyle::LIST;
    }

    // ------------------------------------------------------------------------
    // STEP 4: Initialize Hyprtoolkit Wayland Backend
    // ------------------------------------------------------------------------
    Hyprutils::CLI::CLogger customLogger;
    if (!cli.verbose) {
        customLogger.setLogLevel(Hyprutils::CLI::LOG_CRIT);
        customLogger.setEnableStdout(false);
    }
    IBackend::SBackendCreationData creationData;
    creationData.pLogConnection = makeShared<Hyprutils::CLI::CLoggerConnection>(customLogger);

    auto backend = IBackend::createWithData(creationData);
    if (!backend) {
        backend = IBackend::create();
    }
    if (!backend) {
        std::cerr << "hlmenu: Failed to initialize Hyprtoolkit backend\n";
        return 1;
    }

    // Configure logging handler: suppress noisy debug logs unless verbose is active
    bool isVerbose = cli.verbose;
    backend->setLogFn([isVerbose](eLogLevel level, const std::string& msg) {
        if (!isVerbose) {
            return;
        }

        std::cout << "[hlmenu debug] " << msg << "\n";
    });

    // Synchronize backend toolkit palette with active config
    auto palette = backend->getPalette();
    if (palette) {
        palette->m_colors.background = config.background;
        palette->m_colors.base = config.searchBackground;
        palette->m_colors.alternateBase = config.gridItemActiveBackground;
        palette->m_colors.text = config.searchFontColor;
        palette->m_colors.brightText = config.searchFontColor;
        palette->m_colors.accent = config.primary;
        palette->m_colors.accentSecondary = config.accent;
        palette->m_vars.fontFamily = config.fontFamily;
        palette->m_vars.fontFamilyMonospace = config.monospaceFont;
        palette->m_vars.bigRounding = config.cornerRadiusBig;
        palette->m_vars.smallRounding = config.cornerRadiusSmall;
    }

    // ------------------------------------------------------------------------
    // STEP 5: Load Items & Initialize Filterable ItemList
    // ------------------------------------------------------------------------
    auto items = HLMenu::loadItems(cli, backend);
    auto itemList = std::make_shared<HLMenu::ItemList>();
    itemList->setItems(items);
    if (cli.query && !cli.query->empty()) {
        itemList->setFilter(*cli.query);
    }

    // ------------------------------------------------------------------------
    // STEP 6: Create Layer-Shell Overlay Window
    // ------------------------------------------------------------------------
    // Note on Wayland Layer-Shell:
    //   - `exclusiveZone(-1)` ensures pointer/touch interactivity without stealing workspace tiles.
    //   - `kbInteractive(1)` grants keyboard focus immediately upon map.
    //   - `layer(3)` positions hlmenu above normal windows (Overlay layer).
    // ------------------------------------------------------------------------
    auto window = CWindowBuilder::begin()
        ->type(HT_WINDOW_LAYER)
        ->appTitle("hlmenu")
        ->appClass("hlmenu")
        ->preferredSize(config.windowSize)
        ->marginTopLeft(config.marginTopLeft)
        ->marginBottomRight(config.marginBottomRight)
        ->anchor(config.anchorMask)
        ->layer(3)
        ->kbInteractive(1)
        ->exclusiveZone(-1)
        ->commence();

    if (!window) {
        std::cerr << "hlmenu: Failed to create layer-shell window\n";
        return 1;
    }

    // ------------------------------------------------------------------------
    // STEP 7: Instantiate UI Views (Grid & List)
    // ------------------------------------------------------------------------
    auto listView = std::make_shared<HLMenu::ListView>(backend, config);
    auto gridView = std::make_shared<HLMenu::GridView>(backend, config);

    // Common activation handler: launches item, closes window, exits cleanly
    auto activateHandler = [itemList, &window, backend, &cli](int index) {
        if (cli.format == HLMenu::OutputFormat::INDEX && (cli.mode == HLMenu::MenuMode::OPTIONS || cli.mode == HLMenu::MenuMode::FILES || cli.mode == HLMenu::MenuMode::IMAGES)) {
            std::cout << index << std::endl;
        } else {
            const auto& item = itemList->getItem(index);
            item.activate();
        }
        window->close();
        backend->addIdle([backend]() {
            backend->destroy();
            std::exit(0);
        });
    };

    listView->setOnActivate(activateHandler);
    gridView->setOnActivate(activateHandler);

    if (activeView == HLMenu::ViewStyle::LIST) {
        listView->setItemList(itemList);
    } else {
        gridView->setItemList(itemList);
    }

    // ------------------------------------------------------------------------
    // STEP 8: Construct Window Visual Hierarchy
    // ------------------------------------------------------------------------
    std::atomic<bool> switching{false};
    std::atomic<bool> closing{false};
    HLMenu::ViewStyle currentView = activeView;

    // Root transparent container
    auto root = CRectangleBuilder::begin()
        ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
        ->color([] { return CHyprColor(0, 0, 0, 0); })
        ->commence();
    window->m_rootElement = root;

    // Window Backdrop Rectangle with theme colors, borders, and rounded corners
    CHyprColor bg = config.background;
    CHyprColor borderCol = config.borderColor;
    int borderThick = config.borderThickness;
    int bigRounding = config.cornerRadiusBig;

    auto mainBg = CRectangleBuilder::begin()
        ->color([bg]() { return bg; })
        ->rounding(bigRounding)
        ->borderColor([borderCol]() { return borderCol; })
        ->borderThickness(borderThick)
        ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
        ->commence();
    root->addChild(mainBg);

    // Main Column Layout with window padding
    auto mainColumn = CColumnLayoutBuilder::begin()
        ->gap(0)
        ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
        ->commence();
    mainColumn->setMargin(config.windowPadding);
    mainBg->addChild(mainColumn);

    // Determine multi-mode tab availability
    std::vector<HLMenu::MenuMode> availableModes;
    bool showTabs = false;

    if (cli.modesExplicitlySet) {
        availableModes = cli.modes;
        showTabs = config.showModeTabs && (availableModes.size() > 1);
    } else {
        // Default launch (e.g. `hlmenu`): combine Apps, Run, Windows, and Workspaces
        availableModes = { HLMenu::MenuMode::APPS, HLMenu::MenuMode::RUN, HLMenu::MenuMode::WINDOWS, HLMenu::MenuMode::WORKSPACES };
        showTabs = config.showModeTabs && (cli.mode != HLMenu::MenuMode::OPTIONS) && cli.title.empty();
    }

    // Determine TitleBar and SearchBar visibility
    bool renderTitleBar = config.showTitleBar && (showTabs || !config.topbarTitle.empty());
    bool renderSearchBar = config.showSearchBar;

    float titleBarH = renderTitleBar ? static_cast<float>(config.titleBarHeight > 0 ? config.titleBarHeight : 32) : 0.0F;
    float searchBarH = renderSearchBar ? static_cast<float>(config.searchBarHeight > 0 ? config.searchBarHeight : 38) : 0.0F;
    float titleGap = (renderTitleBar && renderSearchBar) ? static_cast<float>(config.titleBarGap) : 0.0F;
    float searchGap = (renderSearchBar || renderTitleBar) ? static_cast<float>(config.searchBarGap) : 0.0F;

    float innerTotalHeight = static_cast<float>(config.windowSize.y - (config.windowPadding * 2));
    float contentHeight = std::max(50.0F, innerTotalHeight - titleBarH - searchBarH - titleGap - searchGap);

    HLMenu::MenuMode currentMode = cli.mode;

    // 1. TitleBar Component (Header / Mode Switcher Tabs)
    std::shared_ptr<HLMenu::TitleBar> titleBar;
    if (renderTitleBar) {
        auto titleBarContainer = CRectangleBuilder::begin()
            ->color([] { return CHyprColor(0, 0, 0, 0); })
            ->size(CDynamicSize(
                CDynamicSize::HT_SIZE_PERCENT,
                CDynamicSize::HT_SIZE_ABSOLUTE,
                {1.0F, titleBarH}))
            ->commence();

        titleBar = std::make_shared<HLMenu::TitleBar>(backend, config, currentMode, showTabs, availableModes, cli.modeTitles);
        titleBarContainer->addChild(titleBar->getView());
        mainColumn->addChild(titleBarContainer);

        if (titleGap > 0) {
            auto gapSpacer = CRectangleBuilder::begin()
                ->color([] { return CHyprColor(0, 0, 0, 0); })
                ->size(CDynamicSize(
                    CDynamicSize::HT_SIZE_PERCENT,
                    CDynamicSize::HT_SIZE_ABSOLUTE,
                    {1.0F, titleGap}))
                ->commence();
            mainColumn->addChild(gapSpacer);
        }
    }

    // 2. SearchBar Component (Full-width Search Input)
    std::shared_ptr<HLMenu::SearchBar> searchBar;
    if (renderSearchBar) {
        auto searchBarContainer = CRectangleBuilder::begin()
            ->color([] { return CHyprColor(0, 0, 0, 0); })
            ->size(CDynamicSize(
                CDynamicSize::HT_SIZE_PERCENT,
                CDynamicSize::HT_SIZE_ABSOLUTE,
                {1.0F, searchBarH}))
            ->commence();

        searchBar = std::make_shared<HLMenu::SearchBar>(backend, window, config, cli.passwordMode);
        if (cli.query && !cli.query->empty()) {
            searchBar->setText(*cli.query);
        }
        searchBar->onTextChanged([itemList](std::string query) {
            itemList->setFilter(query);
        });
        searchBar->onEnter([&]() {
            if (itemList->visibleCount() > 0) {
                if (currentView == HLMenu::ViewStyle::LIST) {
                    listView->activateCurrentSelection();
                } else {
                    gridView->activateCurrentSelection();
                }
            } else if (currentMode == HLMenu::MenuMode::RUN) {
                std::string q = itemList->getFilter();
                if (!q.empty()) {
                    closing = true;
                    switching = true;
                    std::system((q + " &").c_str());
                    window->close();
                    backend->addIdle([backend]() { backend->destroy(); std::exit(0); });
                }
            } else if (currentMode == HLMenu::MenuMode::OPTIONS || cli.passwordMode) {
                std::string q = itemList->getFilter();
                if (!q.empty()) {
                    closing = true;
                    switching = true;
                    std::cout << q << std::endl;
                    window->close();
                    backend->addIdle([backend]() { backend->destroy(); std::exit(0); });
                }
            }
        });

        searchBarContainer->addChild(searchBar->getView());
        mainColumn->addChild(searchBarContainer);

        if (searchGap > 0) {
            auto gapSpacer = CRectangleBuilder::begin()
                ->color([] { return CHyprColor(0, 0, 0, 0); })
                ->size(CDynamicSize(
                    CDynamicSize::HT_SIZE_PERCENT,
                    CDynamicSize::HT_SIZE_ABSOLUTE,
                    {1.0F, searchGap}))
                ->commence();
            mainColumn->addChild(gapSpacer);
        }
    }

    // 3. Main Content Area (Houses either GridView or ListView)
    auto content = CRectangleBuilder::begin()
        ->color([] { return CHyprColor(0, 0, 0, 0); })
        ->size(CDynamicSize(
            CDynamicSize::HT_SIZE_PERCENT,
            CDynamicSize::HT_SIZE_ABSOLUTE,
            {1.0F, contentHeight}))
        ->commence();

    if (currentView == HLMenu::ViewStyle::LIST) {
        content->addChild(listView->getWidget());
    } else {
        content->addChild(gridView->getWidget());
    }
    mainColumn->addChild(content);

    auto switchMode = [&](HLMenu::MenuMode newMode) {
        if (currentMode == newMode && itemList->totalCount() > 0) return;
        currentMode = newMode;

        HLMenu::CliOptions tempCli = cli;
        tempCli.mode = newMode;

        auto newItems = HLMenu::loadItems(tempCli, backend);
        itemList->setFilter("");
        itemList->setItems(newItems);

        listView->clearItemsCache();
        gridView->clearItemsCache();

        HLMenu::ViewStyle targetView = cli.getViewForMode(newMode);
        if (currentView != targetView) {
            content->clearChildren();
            if (targetView == HLMenu::ViewStyle::LIST) {
                if (!listView->hasItemList()) {
                    listView->setItemList(itemList);
                }
                content->addChild(listView->getWidget());
            } else {
                if (!gridView->hasItemList()) {
                    gridView->setItemList(itemList);
                }
                content->addChild(gridView->getWidget());
            }
            content->forceReposition();
            currentView = targetView;
        }

        if (searchBar) {
            searchBar->clear();
            searchBar->focus();
        }
        if (titleBar) {
            titleBar->setActiveMode(newMode);
        }
    };

    if (titleBar) {
        titleBar->onModeChanged([&](HLMenu::MenuMode m) {
            switchMode(m);
        });
    }

    // ------------------------------------------------------------------------
    // STEP 9: Register Event & Keyboard Listeners
    // ------------------------------------------------------------------------
    auto closeListener = window->m_events.closeRequest.listen([&]() {
        if (closing) return;
        closing = true;
        switching = true;
        backend->addIdle([backend]() {
            backend->destroy();
            std::exit(1);
        });
    });

    auto kbListener = window->m_events.keyboardKey.listen([&](const Input::SKeyboardKeyEvent& e) {
        if (!e.down || closing) return;

        // Escape: Close menu / Ctrl+Escape: Toggle Grid/List view
        if (e.xkbKeysym == XKB_KEY_Escape) {
            if (e.modMask & Input::HT_MODIFIER_CTRL) {
                if (switching || closing) return;
                switching = true;

                HLMenu::ViewStyle newView = (currentView == HLMenu::ViewStyle::GRID) 
                    ? HLMenu::ViewStyle::LIST 
                    : HLMenu::ViewStyle::GRID;

                content->clearChildren();

                if (newView == HLMenu::ViewStyle::LIST) {
                    if (!listView->hasItemList()) {
                        listView->setItemList(itemList);
                    }
                    content->addChild(listView->getWidget());
                } else {
                    if (!gridView->hasItemList()) {
                        gridView->setItemList(itemList);
                    }
                    content->addChild(gridView->getWidget());
                }

                content->forceReposition();
                currentView = newView;
                switching = false;
            } else {
                closing = true;
                switching = true;
                window->close();
                backend->addIdle([backend]() {
                    backend->destroy();
                    std::exit(1);
                });
            }
        } else if (showTabs && (e.modMask & Input::HT_MODIFIER_SHIFT) && (e.xkbKeysym == XKB_KEY_Left || e.xkbKeysym == XKB_KEY_Right)) {
            // Shift+Left / Shift+Right: Navigate between modes in combo mode
            if (!availableModes.empty()) {
                auto it = std::find(availableModes.begin(), availableModes.end(), currentMode);
                size_t idx = (it != availableModes.end()) ? std::distance(availableModes.begin(), it) : 0;

                if (e.xkbKeysym == XKB_KEY_Left) {
                    idx = (idx == 0) ? availableModes.size() - 1 : idx - 1;
                } else {
                    idx = (idx + 1) % availableModes.size();
                }
                switchMode(availableModes[idx]);
            }
        } else if (!switching) {
            // Forward navigation keys to the active view
            if (currentView == HLMenu::ViewStyle::LIST) {
                listView->handleKey(e);
            } else {
                gridView->handleKey(e);
            }
        }
    });

    // ------------------------------------------------------------------------
    // STEP 10: Auto-Focus Search & Enter Event Loop
    // ------------------------------------------------------------------------
    backend->addIdle([searchBar] {
        if (searchBar) {
            searchBar->focus();
        }
    });

    window->open();
    backend->enterLoop();

    return 0;
}
