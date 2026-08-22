#pragma once

#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/element/Image.hpp>
#include <hyprtoolkit/system/Icons.hpp>
#include <hyprutils/memory/SharedPtr.hpp>
#include <hyprutils/math/Vector2D.hpp>
#include <functional>
#include <string>
#include <filesystem>
#include "Core/Item.hpp"
#include "Config/Config.hpp"
#include "Utils/SvgConverter.hpp"

namespace fs = std::filesystem;

namespace HLMenu {

using namespace Hyprutils::Memory;

// ============================================================================
// LIST ITEM - SINGLE-COLUMN ROW WIDGET
// ============================================================================
// Represents a single row in the vertical ListView.
// Layout:
//   [ Icon ]  [ Title (top) / Subtitle (bottom) ]        [ Spacer ]
// ============================================================================

class ListItem {
public:
    /**
     * @brief Constructs a ListItem row widget.
     */
    ListItem(CSharedPointer<Hyprtoolkit::IBackend> backend,
             const Item& data,
             size_t index,
             const HLMenu::MenuConfig& config,
             std::function<void(size_t)> onActivate)
        : m_backend(backend)
        , m_data(data)
        , m_index(index)
        , m_config(config)
        , m_onActivate(onActivate) {
        
        createUI();
        setupMouseHandlers();
    }

    /// @brief Returns the root background rectangle of this row.
    CSharedPointer<Hyprtoolkit::IElement> view() const {
        return m_background;
    }

    /// @brief Returns the root background rectangle of this row.
    CSharedPointer<Hyprtoolkit::IElement> getWidget() const {
        return m_background;
    }

    /// @brief Returns the underlying Item data model.
    const Item& data() const {
        return m_data;
    }

    /// @brief Returns the master index in ItemList.
    size_t index() const {
        return m_index;
    }

    /// @brief Sets whether this row is highlighted/selected.
    void setSelected(bool selected) {
        if (m_selected == selected) return;
        m_selected = selected;
        updateAppearance();
    }

private:
    /**
     * @brief Constructs the row element hierarchy.
     */
    void createUI() {
        float itemH = static_cast<float>(m_config.listItemHeight);
        int rounding = m_config.listItemCornerRadius;
        int padding = m_config.listItemPadding;

        m_mainLayout = Hyprtoolkit::CRowLayoutBuilder::begin()
            ->gap(12)
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                {1.0f, 1.0f}))
            ->commence();
        m_mainLayout->setMargin(padding);

        createIcon();
        if (m_icon) {
            m_icon->setPositionFlag(Hyprtoolkit::IElement::HT_POSITION_FLAG_VCENTER, true);
            m_mainLayout->addChild(m_icon);
        }

        // Text Column: Vertically stacks Title on top and Subtitle on bottom
        m_textColumn = Hyprtoolkit::CColumnLayoutBuilder::begin()
            ->gap(2)
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_AUTO,
                Hyprtoolkit::CDynamicSize::HT_SIZE_AUTO,
                {0.0f, 0.0f}))
            ->commence();
        m_textColumn->setPositionFlag(Hyprtoolkit::IElement::HT_POSITION_FLAG_VCENTER, true);

        createTitle();
        if (m_title) {
            m_textColumn->addChild(m_title);
        }

        if (m_config.showSubtitles && !m_data.subtitle().empty()) {
            createSubtitle();
            if (m_subtitle) {
                m_textColumn->addChild(m_subtitle);
            }
        }

        m_mainLayout->addChild(m_textColumn);

        // Invisible growing spacer pushes elements to the left
        auto spacer = Hyprtoolkit::CRectangleBuilder::begin()
            ->color([] { return Hyprtoolkit::CHyprColor(0,0,0,0); })
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                {1.0f, 1.0f}))
            ->commence();
        spacer->setGrow(true, false);
        m_mainLayout->addChild(spacer);

        // Row background rectangle
        Hyprtoolkit::CHyprColor bg = m_config.listItemBackground;
        Hyprtoolkit::CHyprColor border = m_config.listItemBorderColor;
        int borderSz = m_config.listItemBorderSize;

        m_background = Hyprtoolkit::CRectangleBuilder::begin()
            ->color([bg] { return bg; })
            ->borderColor([border] { return border; })
            ->borderThickness(borderSz)
            ->rounding(rounding)
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
                {1.0f, itemH}))
            ->commence();

        m_background->addChild(m_mainLayout);
    }

    /**
     * @brief Resolves and creates the left-aligned icon image.
     */
    void createIcon() {
        if (m_data.isApp() && std::get<AppItem>(m_data.data).iconDesc) {
            auto builder = Hyprtoolkit::CImageBuilder::begin()
                ->size(Hyprtoolkit::CDynamicSize(
                    Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
                    Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
                    {static_cast<float>(m_config.listItemIconSize), static_cast<float>(m_config.listItemIconSize)}))
                ->fitMode(Hyprtoolkit::IMAGE_FIT_MODE_CONTAIN)
                ->rounding(m_config.cornerRadiusSmall)
                ->sync(false);
            builder->icon(std::get<AppItem>(m_data.data).iconDesc);
            m_icon = builder->commence();
            return;
        }
        if (m_data.isWindow() && std::get<WindowItem>(m_data.data).iconDesc) {
            auto builder = Hyprtoolkit::CImageBuilder::begin()
                ->size(Hyprtoolkit::CDynamicSize(
                    Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
                    Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
                    {static_cast<float>(m_config.listItemIconSize), static_cast<float>(m_config.listItemIconSize)}))
                ->fitMode(Hyprtoolkit::IMAGE_FIT_MODE_CONTAIN)
                ->rounding(m_config.cornerRadiusSmall)
                ->sync(false);
            builder->icon(std::get<WindowItem>(m_data.data).iconDesc);
            m_icon = builder->commence();
            return;
        }
        if (m_data.isWorkspace() && std::get<WorkspaceItem>(m_data.data).iconDesc) {
            auto builder = Hyprtoolkit::CImageBuilder::begin()
                ->size(Hyprtoolkit::CDynamicSize(
                    Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
                    Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
                    {static_cast<float>(m_config.listItemIconSize), static_cast<float>(m_config.listItemIconSize)}))
                ->fitMode(Hyprtoolkit::IMAGE_FIT_MODE_CONTAIN)
                ->rounding(m_config.cornerRadiusSmall)
                ->sync(false);
            builder->icon(std::get<WorkspaceItem>(m_data.data).iconDesc);
            m_icon = builder->commence();
            return;
        }

        std::string iconSource = m_data.iconSource();
        if (iconSource.empty()) {
            m_icon = nullptr;
            return;
        }

        float imageSize = static_cast<float>(m_config.listItemIconSize);
        int iconRounding = m_config.cornerRadiusSmall;

        auto builder = Hyprtoolkit::CImageBuilder::begin()
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
                Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
                {imageSize, imageSize}))
            ->fitMode(Hyprtoolkit::IMAGE_FIT_MODE_CONTAIN)
            ->rounding(iconRounding)
            ->sync(false);

        if (fs::exists(iconSource)) {
            std::string pathToUse = SvgConverter::ensurePngIcon(iconSource, imageSize);
            std::string pathCopy = pathToUse;
            builder->path(std::move(pathCopy));
        } else {
            auto iconFactory = m_backend->systemIcons();
            if (iconFactory) {
                auto iconDesc = iconFactory->lookupIcon(iconSource);
                if (iconDesc && iconDesc->exists()) {
                    builder->icon(iconDesc);
                } else if (!m_data.isOption()) {
                    auto fallback = iconFactory->lookupIcon("application-x-executable");
                    if (fallback && fallback->exists()) {
                        builder->icon(fallback);
                    }
                }
            }
        }

        m_icon = builder->commence();
    }

    /**
     * @brief Creates primary title text label.
     */
    void createTitle() {
        std::string titleText = m_data.displayName();
        Hyprtoolkit::CHyprColor col = m_config.listItemTitleFontColor;
        std::string font = m_config.fontFamily;
        float fontSize = static_cast<float>(m_config.listItemTitleFontSize);

        m_title = Hyprtoolkit::CTextBuilder::begin()
            ->text(std::move(titleText))
            ->color([col] { return col; })
            ->fontFamily(std::move(font))
            ->align(Hyprtoolkit::HT_FONT_ALIGN_LEFT)
            ->fontSize(Hyprtoolkit::CFontSize(Hyprtoolkit::CFontSize::HT_FONT_ABSOLUTE, fontSize))
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_AUTO,
                Hyprtoolkit::CDynamicSize::HT_SIZE_AUTO,
                {0.0f, 0.0f}))
            ->clampSize(Hyprutils::Math::Vector2D(500.0f, 0.0f))
            ->commence();
    }

    /**
     * @brief Creates subtitle text label for applications, files, and windows/workspaces.
     */
    void createSubtitle() {
        std::string sub = m_data.subtitle();
        if (sub.empty()) return;

        Hyprtoolkit::CHyprColor col = m_config.listItemDescFontColor;
        std::string font = m_config.fontFamily;
        float fontSize = static_cast<float>(m_config.listItemDescFontSize);

        m_subtitle = Hyprtoolkit::CTextBuilder::begin()
            ->text(std::move(sub))
            ->color([col] { return col; })
            ->fontFamily(std::move(font))
            ->align(Hyprtoolkit::HT_FONT_ALIGN_LEFT)
            ->fontSize(Hyprtoolkit::CFontSize(Hyprtoolkit::CFontSize::HT_FONT_ABSOLUTE, fontSize))
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_AUTO,
                Hyprtoolkit::CDynamicSize::HT_SIZE_AUTO,
                {0.0f, 0.0f}))
            ->clampSize(Hyprutils::Math::Vector2D(500.0f, 0.0f))
            ->commence();
    }

    /**
     * @brief Attaches mouse click listener.
     */
    void setupMouseHandlers() {
        if (!m_background) return;

        m_background->setReceivesMouse(true);
        m_background->setMouseButton([this](Hyprtoolkit::Input::eMouseButton button, bool down) {
            if (down && button == Hyprtoolkit::Input::MOUSE_BUTTON_LEFT) {
                if (m_onActivate) {
                    m_onActivate(m_index);
                }
            }
        });
    }

    /**
     * @brief Updates appearance when item becomes selected or deselected.
     */
    void updateAppearance() {
        Hyprtoolkit::CHyprColor activeBg = m_config.listItemActiveBackground;
        Hyprtoolkit::CHyprColor inactiveBg = m_config.listItemBackground;
        Hyprtoolkit::CHyprColor activeBorder = m_config.listItemActiveBorderColor;
        Hyprtoolkit::CHyprColor inactiveBorder = m_config.listItemBorderColor;
        int activeBorderSz = m_config.listItemActiveBorderSize;
        int inactiveBorderSz = m_config.listItemBorderSize;

        if (auto builder = m_background->rebuild()) {
            if (m_selected) {
                builder->color([activeBg] { return activeBg; });
                builder->borderColor([activeBorder] { return activeBorder; });
                builder->borderThickness(activeBorderSz);
            } else {
                builder->color([inactiveBg] { return inactiveBg; });
                builder->borderColor([inactiveBorder] { return inactiveBorder; });
                builder->borderThickness(inactiveBorderSz);
            }
            builder->commence();
        }
    }

    CSharedPointer<Hyprtoolkit::IBackend> m_backend;
    CSharedPointer<Hyprtoolkit::CRectangleElement> m_background;
    CSharedPointer<Hyprtoolkit::CRowLayoutElement> m_mainLayout;
    CSharedPointer<Hyprtoolkit::CColumnLayoutElement> m_textColumn;
    CSharedPointer<Hyprtoolkit::CImageElement> m_icon;
    CSharedPointer<Hyprtoolkit::CTextElement> m_title;
    CSharedPointer<Hyprtoolkit::CTextElement> m_subtitle;

    Item m_data;
    size_t m_index;
    HLMenu::MenuConfig m_config;
    bool m_selected = false;

    std::function<void(size_t)> m_onActivate;
};

} // namespace HLMenu
