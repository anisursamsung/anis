#pragma once

#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include "Config/Config.hpp"
#include "Cli/ArgumentParser.hpp"
#include <functional>
#include <string>
#include <vector>

namespace HLMenu {

// ============================================================================
// TITLE BAR - DEDICATED HEADER COMPONENT (MODE TABS & MENU TITLE)
// ============================================================================
// TitleBar manages the top row of hlmenu.
//
// Features:
//   1. Mode Switcher Tabs (Apps, Windows, Workspaces, Files) with glowing pill styling.
//   2. Static Title display for single-purpose menus (e.g. PowerMenu, Active Windows).
//   3. Interactive mouse-click mode selection and active tab highlight.
// ============================================================================

class TitleBar {
public:
    struct ModeTab {
        MenuMode mode;
        std::string label;
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CRectangleElement> pill;
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CTextElement> text;
    };

    /**
     * @brief Constructs the TitleBar component.
     * @param backend Shared pointer to Hyprtoolkit backend.
     * @param config The active MenuConfig settings.
     * @param activeMode Currently active MenuMode.
     * @param showTabs Whether to render mode switcher tabs.
     * @param availableModes List of modes available to switch between.
     */
    TitleBar(
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IBackend> backend,
        const HLMenu::MenuConfig& config,
        MenuMode activeMode = MenuMode::APPS,
        bool showTabs = true,
        const std::vector<MenuMode>& availableModes = {MenuMode::APPS, MenuMode::WINDOWS, MenuMode::WORKSPACES},
        const std::unordered_map<MenuMode, std::string>& modeTitles = {}
    ) : m_backend(backend)
      , m_config(config)
      , m_activeMode(activeMode)
      , m_showTabs(showTabs)
      , m_availableModes(availableModes)
      , m_modeTitles(modeTitles) {
        
        // 1. Root Container Row Layout spanning 100% width
        m_rootLayout = Hyprtoolkit::CRowLayoutBuilder::begin()
            ->gap(8)
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                {1.0F, 1.0F}))
            ->commence();

        if (m_showTabs && !m_availableModes.empty()) {
            createModeTabs();
        } else if (!m_config.topbarTitle.empty()) {
            createStaticTitle();
        }
    }

    /// @brief Returns the root row layout element.
    Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IElement> getView() const {
        return m_rootLayout;
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

private:
    /**
     * @brief Creates interactive tab pills for mode switching.
     */
    void createModeTabs() {
        for (MenuMode mode : m_availableModes) {
            std::string label = getModeLabel(mode);
            bool isActive = (mode == m_activeMode);

            float fontSize = static_cast<float>(m_config.searchFontSize > 0 ? m_config.searchFontSize : 13);
            std::string font = m_config.fontFamily;
            int rounding = m_config.cornerRadiusSmall;

            Hyprtoolkit::CHyprColor activeCol = m_config.topbarTitleFontColor;
            Hyprtoolkit::CHyprColor textCol = m_config.searchFontColor;

            // Tab Text Label
            std::string labelCopy = label;
            auto textElem = Hyprtoolkit::CTextBuilder::begin()
                ->text(std::move(labelCopy))
                ->color([textCol] { return textCol; })
                ->fontFamily(std::move(font))
                ->align(Hyprtoolkit::HT_FONT_ALIGN_CENTER)
                ->fontSize(Hyprtoolkit::CFontSize(Hyprtoolkit::CFontSize::HT_FONT_ABSOLUTE, fontSize))
                ->size(Hyprtoolkit::CDynamicSize(
                    Hyprtoolkit::CDynamicSize::HT_SIZE_AUTO,
                    Hyprtoolkit::CDynamicSize::HT_SIZE_AUTO,
                    {0.0F, 0.0F}))
                ->commence();

            // Inner Pill Layout
            auto pillLayout = Hyprtoolkit::CRowLayoutBuilder::begin()
                ->gap(0)
                ->size(Hyprtoolkit::CDynamicSize(
                    Hyprtoolkit::CDynamicSize::HT_SIZE_AUTO,
                    Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                    {0.0F, 1.0F}))
                ->commence();
            pillLayout->setMargin(4);

            if (textElem) {
                textElem->setPositionFlag(Hyprtoolkit::IElement::HT_POSITION_FLAG_VCENTER, true);
                pillLayout->addChild(textElem);
            }

            // Pill Background Rectangle
            Hyprtoolkit::CHyprColor activeBg = Hyprtoolkit::CHyprColor(activeCol.r, activeCol.g, activeCol.b, 0.25f);
            Hyprtoolkit::CHyprColor inactiveBg = Hyprtoolkit::CHyprColor(0.0f, 0.0f, 0.0f, 0.0f);
            Hyprtoolkit::CHyprColor bgCol = isActive ? activeBg : inactiveBg;

            Hyprtoolkit::CHyprColor activeBorder = activeCol;
            Hyprtoolkit::CHyprColor inactiveBorder = Hyprtoolkit::CHyprColor(0.0f, 0.0f, 0.0f, 0.0f);
            Hyprtoolkit::CHyprColor borderCol = isActive ? activeBorder : inactiveBorder;

            auto pill = Hyprtoolkit::CRectangleBuilder::begin()
                ->color([bgCol] { return bgCol; })
                ->borderColor([borderCol] { return borderCol; })
                ->borderThickness(isActive ? 1 : 0)
                ->rounding(rounding)
                ->size(Hyprtoolkit::CDynamicSize(
                    Hyprtoolkit::CDynamicSize::HT_SIZE_AUTO,
                    Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                    {0.0F, 1.0F}))
                ->commence();

            pill->addChild(pillLayout);
            pill->setPositionFlag(Hyprtoolkit::IElement::HT_POSITION_FLAG_VCENTER, true);
            pill->setReceivesMouse(true);

            pill->setMouseButton([this, mode](Hyprtoolkit::Input::eMouseButton btn, bool down) {
                if (down && btn == Hyprtoolkit::Input::MOUSE_BUTTON_LEFT) {
                    if (m_onModeChanged) {
                        m_onModeChanged(mode);
                    }
                }
            });

            m_rootLayout->addChild(pill);
            m_tabs.push_back({mode, label, pill, textElem});
        }
    }

    /**
     * @brief Creates a static text title element.
     */
    void createStaticTitle() {
        std::string titleText = m_config.topbarTitle;
        Hyprtoolkit::CHyprColor col = m_config.topbarTitleFontColor;
        std::string font = m_config.fontFamily;
        float fontSize = static_cast<float>(m_config.topbarTitleFontSize);

        m_staticTitle = Hyprtoolkit::CTextBuilder::begin()
            ->text(std::move(titleText))
            ->color([col] { return col; })
            ->fontFamily(std::move(font))
            ->align(Hyprtoolkit::HT_FONT_ALIGN_LEFT)
            ->fontSize(Hyprtoolkit::CFontSize(Hyprtoolkit::CFontSize::HT_FONT_ABSOLUTE, fontSize))
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                Hyprtoolkit::CDynamicSize::HT_SIZE_AUTO,
                {1.0F, 0.0F}))
            ->commence();

        if (m_staticTitle) {
            m_staticTitle->setPositionFlag(Hyprtoolkit::IElement::HT_POSITION_FLAG_VCENTER, true);
            m_rootLayout->addChild(m_staticTitle);
        }
    }

    /**
     * @brief Refreshes pill background and border on active mode change.
     */
    void updateTabStyles() {
        Hyprtoolkit::CHyprColor activeBorder = m_config.topbarTitleFontColor;
        Hyprtoolkit::CHyprColor activeBg = Hyprtoolkit::CHyprColor(activeBorder.r, activeBorder.g, activeBorder.b, 0.25f);
        Hyprtoolkit::CHyprColor inactiveBg = Hyprtoolkit::CHyprColor(0.0f, 0.0f, 0.0f, 0.0f);
        Hyprtoolkit::CHyprColor inactiveBorder = Hyprtoolkit::CHyprColor(0.0f, 0.0f, 0.0f, 0.0f);

        for (auto& tab : m_tabs) {
            bool isActive = (tab.mode == m_activeMode);

            Hyprtoolkit::CHyprColor bgCol = isActive ? activeBg : inactiveBg;
            Hyprtoolkit::CHyprColor borderCol = isActive ? activeBorder : inactiveBorder;

            if (tab.pill) {
                if (auto builder = tab.pill->rebuild()) {
                    builder->color([bgCol] { return bgCol; })
                           ->borderColor([borderCol] { return borderCol; })
                           ->borderThickness(isActive ? 1 : 0)
                           ->commence();
                }
            }
        }
    }

    std::string getModeLabel(MenuMode mode) const {
        auto it = m_modeTitles.find(mode);
        if (it != m_modeTitles.end() && !it->second.empty()) {
            return " " + it->second + " ";
        }
        if (mode == MenuMode::OPTIONS && !m_config.topbarTitle.empty() && m_config.topbarTitle != "Applications") {
            return " " + m_config.topbarTitle + " ";
        }
        switch (mode) {
            case MenuMode::APPS: return " Apps ";
            case MenuMode::RUN: return " Run ";
            case MenuMode::WINDOWS: return " Windows ";
            case MenuMode::WORKSPACES: return " Workspaces ";
            case MenuMode::FILES: return " Files ";
            case MenuMode::IMAGES: return " Images ";
            case MenuMode::OPTIONS: return " Options ";
            default: return " Menu ";
        }
    }

    Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IBackend> m_backend;
    HLMenu::MenuConfig m_config;
    MenuMode m_activeMode;
    bool m_showTabs;
    std::vector<MenuMode> m_availableModes;
    std::unordered_map<MenuMode, std::string> m_modeTitles;

    Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CRowLayoutElement> m_rootLayout;
    Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CTextElement> m_staticTitle;
    std::vector<ModeTab> m_tabs;

    std::function<void(MenuMode)> m_onModeChanged;
};

} // namespace HLMenu
