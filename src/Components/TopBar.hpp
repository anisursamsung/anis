#pragma once

#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <hyprtoolkit/element/Textbox.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/core/Timer.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include "Config/Config.hpp"
#include "Cli/ArgumentParser.hpp"
#include <xkbcommon/xkbcommon-keysyms.h>
#include <chrono>
#include <functional>
#include <algorithm>
#include <vector>

namespace HLMenu {

// ============================================================================
// TOP BAR - HEADER COMPONENT (MENU TITLE, MODE TABS & SEARCH INPUT)
// ============================================================================
// TopBar manages the header area of hlmenu.
//
// Features:
//   1. Left Section: Interactive Mode Tabs (Apps, Windows, Workspaces, Files)
//      OR classic static title for single-purpose menus (e.g. PowerMenu).
//   2. Right Section: Search textbox taking the remaining width.
//   3. Debounced input filtering (100ms timer) to prevent lag on fast typing.
//   4. Global Enter key listener routing selection to views.
// ============================================================================

class TopBar {
public:
    struct ModeTab {
        MenuMode mode;
        std::string label;
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CRectangleElement> pill;
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CTextElement> text;
    };

    /**
     * @brief Constructs the TopBar component.
     * @param backend Shared pointer to Hyprtoolkit backend.
     * @param window Shared pointer to parent layer window for key event listening.
     * @param config The active MenuConfig settings.
     * @param activeMode Currently active MenuMode.
     * @param showTabs Whether to render mode switcher tabs.
     * @param availableModes List of modes available to switch between.
     */
    TopBar(
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IBackend> backend,
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IWindow> window,
        const HLMenu::MenuConfig& config,
        MenuMode activeMode = MenuMode::APPS,
        bool showTabs = true,
        const std::vector<MenuMode>& availableModes = {MenuMode::APPS, MenuMode::WINDOWS, MenuMode::WORKSPACES}
    ) : m_backend(backend)
      , m_config(config)
      , m_activeMode(activeMode)
      , m_showTabs(showTabs)
      , m_availableModes(availableModes) {
        
        // 1. Root Container Row Layout spanning full header bounds
        m_rootLayout = Hyprtoolkit::CRowLayoutBuilder::begin()
            ->gap(12)
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                {1.0F, 1.0F}))
            ->commence();

        // Calculate horizontal split ratio
        float titleRatio = std::clamp(m_config.topbarTitleRatio, 0.0f, 0.7f);
        if (m_showTabs && titleRatio < 0.40f) {
            titleRatio = 0.45f; // Allocate enough room for mode tabs
        }
        float searchRatio = 1.0f - titleRatio;

        // 2. Left Section: Mode Switcher Tabs OR Static Title
        if (m_showTabs && !m_availableModes.empty()) {
            createModeTabs(titleRatio);
        } else if (titleRatio > 0.01f && !m_config.topbarTitle.empty()) {
            createStaticTitle(titleRatio);
        }

        // 3. Right Section: Search Input Field
        std::string placeholder = m_config.searchPlaceholder;
        m_searchInput = Hyprtoolkit::CTextboxBuilder::begin()
            ->placeholder(std::move(placeholder))
            ->defaultText("")
            ->multiline(false)
            ->onTextEdited([this](auto, const std::string& t) { onTextChanged(t); })
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                {searchRatio, 1.0F}))
            ->commence();

        if (m_searchInput) {
            m_searchInput->setPositionFlag(Hyprtoolkit::IElement::HT_POSITION_FLAG_VCENTER, true);
            m_rootLayout->addChild(m_searchInput);
        }

        // 4. Keyboard Listener for Enter Key
        m_listener = window->m_events.keyboardKey.listen([this](const Hyprtoolkit::Input::SKeyboardKeyEvent& e) {
            if (e.down && e.xkbKeysym == XKB_KEY_Return) {
                if (m_onEnter) {
                    m_onEnter();
                }
                scheduleClear();
            }
        });
    }

    /// @brief Returns the root row layout element.
    Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IElement> getView() const {
        return m_rootLayout;
    }

    /// @brief Focuses keyboard input into the search textbox.
    void focus() {
        if (m_searchInput) {
            m_searchInput->focus(true);
        }
    }

    /// @brief Registers callback for debounced text change events.
    void onTextChanged(std::function<void(std::string)> cb) {
        m_onTextChanged = cb;
    }

    /// @brief Registers callback for Enter key activations.
    void onEnter(std::function<void()> cb) {
        m_onEnter = cb;
    }

    /// @brief Registers callback when a mode tab is selected.
    void onModeChanged(std::function<void(MenuMode)> cb) {
        m_onModeChanged = cb;
    }

    /// @brief Updates the active mode and refreshes tab styling.
    void setActiveMode(MenuMode mode) {
        if (m_activeMode == mode) return;
        m_activeMode = mode;
        updateTabStyles();
    }

    /// @brief Clears the active search query.
    void clear() {
        if (m_timer) m_timer->cancel();
        if (m_searchInput) {
            if (auto builder = m_searchInput->rebuild()) {
                builder->defaultText("")->commence();
            }
        }
    }

private:
    /**
     * @brief Creates interactive tab pills for mode switching.
     */
    void createModeTabs(float titleRatio) {
        auto tabsContainer = Hyprtoolkit::CRowLayoutBuilder::begin()
            ->gap(6)
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                {titleRatio, 1.0F}))
            ->commence();

        tabsContainer->setPositionFlag(Hyprtoolkit::IElement::HT_POSITION_FLAG_VCENTER, true);

        for (MenuMode mode : m_availableModes) {
            std::string label = getModeLabel(mode);
            bool isActive = (mode == m_activeMode);

            float fontSize = static_cast<float>(m_config.searchFontSize > 0 ? m_config.searchFontSize : 13);
            std::string font = m_config.fontFamily;
            int rounding = m_config.cornerRadiusSmall;

            Hyprtoolkit::CHyprColor activeCol = m_config.topbarTitleFontColor;
            Hyprtoolkit::CHyprColor inactiveCol = m_config.listItemDescFontColor;

            // Tab Text Label
            std::string labelCopy = label;
            auto textElem = Hyprtoolkit::CTextBuilder::begin()
                ->text(std::move(labelCopy))
                ->color([isActive, activeCol, inactiveCol] { return isActive ? activeCol : inactiveCol; })
                ->fontFamily(std::move(font))
                ->align(Hyprtoolkit::HT_FONT_ALIGN_CENTER)
                ->fontSize(Hyprtoolkit::CFontSize(Hyprtoolkit::CFontSize::HT_FONT_ABSOLUTE, fontSize))
                ->size(Hyprtoolkit::CDynamicSize(
                    Hyprtoolkit::CDynamicSize::HT_SIZE_AUTO,
                    Hyprtoolkit::CDynamicSize::HT_SIZE_AUTO,
                    {0.0F, 0.0F}))
                ->commence();

            // Tab Container Pill
            auto pillElem = Hyprtoolkit::CRectangleBuilder::begin()
                ->color([isActive, activeCol] {
                    return isActive 
                        ? Hyprtoolkit::CHyprColor(activeCol.r, activeCol.g, activeCol.b, 0.20f)
                        : Hyprtoolkit::CHyprColor(0, 0, 0, 0);
                })
                ->borderColor([isActive, activeCol] {
                    return isActive ? activeCol : Hyprtoolkit::CHyprColor(0, 0, 0, 0);
                })
                ->borderThickness(isActive ? 1 : 0)
                ->rounding(rounding)
                ->size(Hyprtoolkit::CDynamicSize(
                    Hyprtoolkit::CDynamicSize::HT_SIZE_AUTO,
                    Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                    {0.0F, 0.75F}))
                ->commence();

            pillElem->setMargin(6);
            pillElem->addChild(textElem);

            // Register mouse click listener
            pillElem->setReceivesMouse(true);
            pillElem->setMouseButton([this, mode](Hyprtoolkit::Input::eMouseButton button, bool down) {
                if (down && button == Hyprtoolkit::Input::MOUSE_BUTTON_LEFT) {
                    if (m_onModeChanged) {
                        m_onModeChanged(mode);
                    }
                }
            });

            ModeTab tab;
            tab.mode = mode;
            tab.label = label;
            tab.pill = pillElem;
            tab.text = textElem;

            m_tabs.push_back(tab);
            tabsContainer->addChild(pillElem);
        }

        m_rootLayout->addChild(tabsContainer);
    }

    /**
     * @brief Refreshes tab visual styling when active mode changes.
     */
    void updateTabStyles() {
        for (auto& tab : m_tabs) {
            bool isActive = (tab.mode == m_activeMode);
            Hyprtoolkit::CHyprColor activeCol = m_config.topbarTitleFontColor;
            Hyprtoolkit::CHyprColor inactiveCol = m_config.listItemDescFontColor;

            if (tab.pill) {
                if (auto builder = tab.pill->rebuild()) {
                    builder->color([isActive, activeCol] {
                        return isActive 
                            ? Hyprtoolkit::CHyprColor(activeCol.r, activeCol.g, activeCol.b, 0.20f)
                            : Hyprtoolkit::CHyprColor(0, 0, 0, 0);
                    })
                    ->borderColor([isActive, activeCol] {
                        return isActive ? activeCol : Hyprtoolkit::CHyprColor(0, 0, 0, 0);
                    })
                    ->borderThickness(isActive ? 1 : 0)
                    ->commence();
                }
            }

            if (tab.text) {
                if (auto builder = tab.text->rebuild()) {
                    builder->color([isActive, activeCol, inactiveCol] {
                        return isActive ? activeCol : inactiveCol;
                    })->commence();
                }
            }
        }
    }

    /**
     * @brief Creates a static title text label.
     */
    void createStaticTitle(float titleRatio) {
        std::string titleStr = m_config.topbarTitle;
        Hyprtoolkit::CHyprColor titleCol = m_config.topbarTitleFontColor;
        float titleFontSize = static_cast<float>(m_config.topbarTitleFontSize);
        std::string font = m_config.fontFamily;

        m_titleText = Hyprtoolkit::CTextBuilder::begin()
            ->text(std::move(titleStr))
            ->color([titleCol] { return titleCol; })
            ->fontFamily(std::move(font))
            ->align(Hyprtoolkit::HT_FONT_ALIGN_LEFT)
            ->fontSize(Hyprtoolkit::CFontSize(Hyprtoolkit::CFontSize::HT_FONT_ABSOLUTE, titleFontSize))
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                {titleRatio, 1.0F}))
            ->commence();

        if (m_titleText) {
            m_titleText->setPositionFlag(Hyprtoolkit::IElement::HT_POSITION_FLAG_VCENTER, true);
            m_rootLayout->addChild(m_titleText);
        }
    }

    static std::string getModeLabel(MenuMode mode) {
        switch (mode) {
            case MenuMode::APPS: return "Apps";
            case MenuMode::WINDOWS: return "Windows";
            case MenuMode::WORKSPACES: return "Workspaces";
            case MenuMode::FILES: return "Files";
            case MenuMode::IMAGES: return "Images";
            case MenuMode::OPTIONS: return "Options";
            default: return "Menu";
        }
    }

    /**
     * @brief Debounces text input edits by 100ms before notifying listeners.
     */
    void onTextChanged(const std::string& text) {
        if (m_timer) m_timer->cancel();

        // Strip any tab characters that CTextboxElement may insert on Ctrl+Tab
        std::string clean = text;
        clean.erase(std::remove(clean.begin(), clean.end(), '\t'), clean.end());

        m_timer = m_backend->addTimer(
            std::chrono::milliseconds(100),
            [this, clean](auto, void*) {
                if (m_onTextChanged) m_onTextChanged(clean);
            },
            nullptr, false
        );
    }

    /**
     * @brief Clears search textbox asynchronously after launch.
     */
    void scheduleClear() {
        m_clearTimer = m_backend->addTimer(
            std::chrono::milliseconds(50),
            [this](auto, void*) {
                clear();
            },
            nullptr, false
        );
    }

    Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IBackend> m_backend;
    HLMenu::MenuConfig m_config;
    MenuMode m_activeMode = MenuMode::APPS;
    bool m_showTabs = true;
    std::vector<MenuMode> m_availableModes;
    std::vector<ModeTab> m_tabs;

    Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CRowLayoutElement> m_rootLayout;
    Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CTextElement> m_titleText;
    Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CTextboxElement> m_searchInput;

    Hyprutils::Memory::CAtomicSharedPointer<Hyprtoolkit::CTimer> m_timer;
    Hyprutils::Memory::CAtomicSharedPointer<Hyprtoolkit::CTimer> m_clearTimer;
    Hyprutils::Memory::CSharedPointer<Hyprutils::Signal::CSignalListener> m_listener;

    std::function<void(std::string)> m_onTextChanged;
    std::function<void()> m_onEnter;
    std::function<void(MenuMode)> m_onModeChanged;
};

} // namespace HLMenu
