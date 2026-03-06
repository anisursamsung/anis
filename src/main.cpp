#include "Collection.hpp"
#include "ListView.hpp"
#include "GridView.hpp"
#include "SearchBox.hpp"
#include "HeaderButton.hpp"
#include "ListApps.hpp"
#include "FileBrowser.hpp"
#include "IconResolver.hpp"
#include "OptionParser.hpp"

#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <iostream>
#include <atomic>
#include <chrono>
#include <string>
#include <algorithm>
#include <cstring>

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

// -----------------------------------------------------------------
// ENUMS
// -----------------------------------------------------------------
enum class Mode { APPS, FILES, OPTIONS };
enum class View { LIST, GRID };

// -----------------------------------------------------------------
// GLOBAL SETTINGS
// -----------------------------------------------------------------
Mode CURRENT_MODE = Mode::APPS;
View CURRENT_VIEW = View::LIST;
std::string SOURCE = "";
std::string ONCLICK = "";  // NEW: global onClick command

// -----------------------------------------------------------------
// HELPER FUNCTIONS
// -----------------------------------------------------------------
std::string toLower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

void printHelp(const char* appName) {
    std::cout << "Usage: " << appName << " [options]\n"
              << "\nOptions:\n"
              << "  -mode <apps|files|options>    Mode to run in (default: apps)\n"
              << "  -view <list|grid>             View style (default: list)\n"
              << "  -source <path|string>         Source for items\n"
              << "  -onclick <command>            Custom command to run on click (use %f for file, %n for name, %a for action)\n"
              << "  -h, -help                      Show this help\n"
              << "\nModes:\n"
              << "  apps                           Application launcher (no source needed)\n"
              << "  files                          File browser (source = directory path)\n"
              << "  options                        Custom menu (source = item list)\n"
              << "\nOptions Mode Formats:\n"
              << "  Simple list:                   \"Save,Load,Quit\"\n"
              << "  With icons:                    \"icon.png,Name;icon2.png,Name2\"\n"
              << "  With commands:                  \"icon.png,Name,command;icon2.png,Name2,command2\"\n"
              << "\nOnClick Placeholders:\n"
              << "  %f                             Replaced with file path (files mode only)\n"
              << "  %n                             Replaced with item name\n"
              << "  %a                             Replaced with action (options mode only)\n"
              << "\nExamples:\n"
              << "  " << appName << "\n"
              << "  " << appName << " -mode apps -view grid\n"
              << "  " << appName << " -mode files -source ~/Pictures\n"
              << "  " << appName << " -mode files -source ~/Wallpapers -onclick \"set-wallpaper.sh %f\"\n"
              << "  " << appName << " -mode options -source \"Save,Load,Quit\" -onclick \"echo Selected: %n\"\n"
              << "  " << appName << " -mode options -source \"firefox.png,Firefox,/usr/bin/firefox;code.png,VS Code,/usr/bin/code\"\n";
}

void parseArguments(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        std::string lowerArg = toLower(arg);
        
        if (lowerArg == "-h" || lowerArg == "-help" || lowerArg == "--help") {
            printHelp(argv[0]);
            exit(0);
        }
        else if (lowerArg == "-mode") {
            if (i + 1 < argc) {
                std::string mode = toLower(argv[++i]);
                if (mode == "apps") CURRENT_MODE = Mode::APPS;
                else if (mode == "files") CURRENT_MODE = Mode::FILES;
                else if (mode == "options") CURRENT_MODE = Mode::OPTIONS;
                else {
                    std::cerr << "Invalid mode: " << mode << std::endl;
                    exit(1);
                }
            }
        }
        else if (lowerArg == "-view") {
            if (i + 1 < argc) {
                std::string view = toLower(argv[++i]);
                if (view == "list") CURRENT_VIEW = View::LIST;
                else if (view == "grid") CURRENT_VIEW = View::GRID;
                else {
                    std::cerr << "Invalid view: " << view << std::endl;
                    exit(1);
                }
            }
        }
        else if (lowerArg == "-source") {
            if (i + 1 < argc) {
                SOURCE = argv[++i];
            }
        }
        else if (lowerArg == "-onclick" || lowerArg == "--onclick") {
            if (i + 1 < argc) {
                ONCLICK = argv[++i];
            }
        }
        else if (arg[0] == '-') {
            std::cerr << "Unknown option: " << arg << std::endl;
            printHelp(argv[0]);
            exit(1);
        }
    }
    
    // Validate mode and source
    if (CURRENT_MODE == Mode::FILES && SOURCE.empty()) {
        std::cerr << "Error: -mode files requires -source <directory>" << std::endl;
        exit(1);
    }
    if (CURRENT_MODE == Mode::OPTIONS && SOURCE.empty()) {
        std::cerr << "Error: -mode options requires -source <item list>" << std::endl;
        exit(1);
    }
}

std::vector<Item> loadItems(Mode mode, const std::string& source, const std::string& onClick, CSharedPointer<IBackend> backend) {
    switch (mode) {
        case Mode::APPS: {
            if (source.empty()) {
                return ListApps::getApps(backend);
            } else {
                return ListApps::getAppsFromDir(source, backend);
            }
        }
        
        case Mode::FILES: {
            return FileBrowser::getImages(source, onClick);
        }
        
        case Mode::OPTIONS: {
            return OptionParser::parseOptions(source, onClick);
        }
        
        default:
            return {};
    }
}

CSharedPointer<CRectangleElement> createSpacer(float w) {
    return CRectangleBuilder::begin()
        ->color([] { return CHyprColor(0,0,0,0); })
        ->size(CDynamicSize(
            CDynamicSize::HT_SIZE_PERCENT,
            CDynamicSize::HT_SIZE_PERCENT,
            {w, 1.0F}))
        ->commence();
}

// -----------------------------------------------------------------
// MAIN
// -----------------------------------------------------------------
int main(int argc, char* argv[]) {
    // Parse arguments
    parseArguments(argc, argv);
    
    // Create backend
    auto backend = IBackend::create();
    if (!backend) return 1;
    
    // Get palette
    auto palette = CPalette::palette();
    int bigRounding = palette ? palette->m_vars.bigRounding : 10;
    
    // Load items based on mode
    auto items = loadItems(CURRENT_MODE, SOURCE, ONCLICK, backend);
    
    // Create collection
    auto collection = std::make_shared<Collection>();
    collection->setItems(items);
    
    // Create window
    auto window = CWindowBuilder::begin()
        ->type(HT_WINDOW_LAYER)
        ->appTitle("anis")
        ->appClass("anis")
        ->preferredSize({1200, 800})
        ->minSize({600, 400})
        ->anchor(1 | 2 | 4 | 8)
        ->layer(3)
        ->kbInteractive(1)
        ->exclusiveZone(-1)
        ->commence();
    
    // Create views
    auto listView = std::make_shared<ListView>(backend);
    auto gridView = std::make_shared<GridView>(backend);
    
    // Set activation handlers
   // Set activation handlers
auto activateHandler = [collection, &window, backend](int index) {
    const auto& item = collection->getItem(index);
  
    
    // Activate the item (this may echo or run a command)
    item.activate();
    
    // Close the window
    window->close();
    
    // Schedule backend destruction and exit
    // This ensures anis completely terminates
    backend->addIdle([backend]() { 
        backend->destroy();
        exit(0);  // Force exit to be safe
    });
};
    listView->setOnActivate(activateHandler);
    gridView->setOnActivate(activateHandler);
    
    // Set collections
    listView->setCollection(collection);
    gridView->setCollection(collection);
    
    // State
    std::atomic<bool> switching{false};
    std::atomic<bool> closing{false};
    View currentView = CURRENT_VIEW;
    
    // Build UI
    auto root = CRectangleBuilder::begin()
        ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.0F,1.0F}))
        ->color([] { return CHyprColor(0,0,0,0); })
        ->commence();
    window->m_rootElement = root;
    
    auto mainBg = CRectangleBuilder::begin()
        ->color([p=palette] { return p->m_colors.background; })
        ->rounding(bigRounding)
        ->borderColor([p=palette] { return p->m_colors.accent.darken(0.2); })
        ->borderThickness(0)
        ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.0F,1.0F}))
        ->commence();
    root->addChild(mainBg);
    
    auto mainColumn = CColumnLayoutBuilder::begin()
        ->gap(0)
        ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.0F,1.0F}))
        ->commence();
    mainColumn->setMargin(10);
    mainBg->addChild(mainColumn);
    
    // Header
    auto header = CRectangleBuilder::begin()
        ->color([] { return CHyprColor(0,0,0,0); })
        ->size(CDynamicSize(
            CDynamicSize::HT_SIZE_PERCENT, 
            CDynamicSize::HT_SIZE_PERCENT, 
            {1.0F, 0.05F}))
        ->commence();
    
    auto searchBox = std::make_shared<SearchBox>(backend, window);
    searchBox->onTextChanged([collection](std::string t) { 
      
        collection->setFilter(t); 
    });
    searchBox->onEnter([&]() { 
       
        if (currentView == View::LIST) {
            listView->activateCurrentSelection();
        } else {
            gridView->activateCurrentSelection();
        }
    });
    
    header->addChild(searchBox->getView());
    mainColumn->addChild(header);
    
    // Content
    auto content = CRectangleBuilder::begin()
        ->color([] { return CHyprColor(0,0,0,0); })
        ->size(CDynamicSize(
            CDynamicSize::HT_SIZE_PERCENT, 
            CDynamicSize::HT_SIZE_PERCENT, 
            {1.0F, 0.95F}))
        ->commence();
    
    if (CURRENT_VIEW == View::LIST) {
        content->addChild(listView->getWidget());
    } else {
        content->addChild(gridView->getWidget());
    }
    mainColumn->addChild(content);
    
    // Event handlers
    auto closeListener = window->m_events.closeRequest.listen([&]() {
        if (closing) return;
        closing = true;
        switching = true;
       
        backend->addIdle([backend]() { backend->destroy(); });
    });
    
    auto kbListener = window->m_events.keyboardKey.listen([&](const Input::SKeyboardKeyEvent& e) {
        if (!e.down || closing) return;
        
        if (e.xkbKeysym == XKB_KEY_Escape) {
            if (e.modMask & Input::HT_MODIFIER_CTRL) {
                if (switching || closing) return;
                switching = true;
                
                View newView = (currentView == View::GRID) ? View::LIST : View::GRID;
              
                
                content->clearChildren();
                
                if (newView == View::LIST) {
                    content->addChild(listView->getWidget());
                } else {
                    content->addChild(gridView->getWidget());
                }
                
                content->forceReposition();
                currentView = newView;
                switching = false;
            } else {
                
                closing = true;
                switching = true;
                window->close();
                backend->addIdle([backend]() { backend->destroy(); });
            }
        } else if (!switching) {
            if (currentView == View::LIST) {
                listView->handleKey(e);
            } else {
                gridView->handleKey(e);
            }
        }
    });
    
    // Focus search box on start
    backend->addIdle([searchBox] { 
        searchBox->focus(); 
    });
    
    // Run
    window->open();
    backend->enterLoop();
    
    return 0;
}
