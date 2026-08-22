#pragma once

#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/element/Image.hpp>
#include <hyprtoolkit/system/Icons.hpp>
#include <hyprutils/memory/SharedPtr.hpp>
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
// GRID ITEM - CARD-STYLED TILE WIDGET
// ============================================================================
// Represents a single application, file, or option tile in the GridView.
// Sizing Behavior:
//   - Without Subtitle: Prominent large icon + centered title below with natural gap.
//   - With Subtitle: Balanced icon + title + small subtitle stack.
// ============================================================================

class GridItem {
public:
    /**
     * @brief Constructs a GridItem card widget.
     * @param backend Shared pointer to Hyprtoolkit backend for icon lookups.
     * @param data The underlying Item data model.
     * @param index Master index of this item in the ItemList.
     * @param config The active MenuConfig settings.
     * @param onActivate Callback invoked when tile is clicked or activated.
     */
    GridItem(CSharedPointer<Hyprtoolkit::IBackend> backend,
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

    /// @brief Returns the root background rectangle of this tile.
    CSharedPointer<Hyprtoolkit::IElement> view() const {
        return m_background;
    }

    /// @brief Returns the root background rectangle of this tile.
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

    /// @brief Sets whether this tile is currently selected/highlighted.
    void setSelected(bool selected) {
        if (m_selected == selected) return;
        m_selected = selected;
        updateAppearance();
    }

private:
    bool hasSubtitle() const {
        return m_config.showSubtitles && !m_data.subtitle().empty();
    }

    /**
     * @brief Builds the initial visual hierarchy for the tile.
     */
    void createUI() {
        float itemW = static_cast<float>(m_config.gridItemWidth);
        float itemH = static_cast<float>(m_config.gridItemHeight);
        int rounding = m_config.gridItemCornerRadius;
        int padding = m_config.gridItemPadding;

        bool hasSub = hasSubtitle();
        int gap = hasSub ? 3 : 8;

        // Content layout containing icon, title, and optional subtitle
        m_contentLayout = Hyprtoolkit::CColumnLayoutBuilder::begin()
            ->gap(gap)
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                Hyprtoolkit::CDynamicSize::HT_SIZE_AUTO,
                {1.0f, 0.0f}))
            ->commence();
        m_contentLayout->setMargin(padding);
        m_contentLayout->setPositionFlag(Hyprtoolkit::IElement::HT_POSITION_FLAG_VCENTER, true);
        m_contentLayout->setPositionFlag(Hyprtoolkit::IElement::HT_POSITION_FLAG_HCENTER, true);

        createImage(hasSub);
        if (m_image) {
            m_image->setPositionFlag(Hyprtoolkit::IElement::HT_POSITION_FLAG_HCENTER, true);
            m_contentLayout->addChild(m_image);
        }

        createTitle();
        if (m_title) {
            m_contentLayout->addChild(m_title);
        }

        if (hasSub) {
            createSubtitle();
            if (m_subtitle) {
                m_contentLayout->addChild(m_subtitle);
            }
        }

        // Tile background rectangle with absolute sizing
        Hyprtoolkit::CHyprColor bg = m_config.gridItemBackground;
        Hyprtoolkit::CHyprColor border = m_config.gridItemBorderColor;
        int borderSz = m_config.gridItemBorderSize;

        m_background = Hyprtoolkit::CRectangleBuilder::begin()
            ->color([bg] { return bg; })
            ->borderColor([border] { return border; })
            ->borderThickness(borderSz)
            ->rounding(rounding)
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
                Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
                {itemW, itemH}))
            ->commence();

        m_background->addChild(m_contentLayout);
    }

    /**
     * @brief Resolves and creates the centered icon image.
     */
    void createImage(bool hasSub) {
        if (m_data.isApp() && std::get<AppItem>(m_data.data).iconDesc) {
            float baseIconSize = static_cast<float>(m_config.gridItemIconSize);
            float imageSize = hasSub ? (baseIconSize * 0.85f) : baseIconSize;
            int iconRounding = m_config.cornerRadiusSmall;

            auto builder = Hyprtoolkit::CImageBuilder::begin()
                ->size(Hyprtoolkit::CDynamicSize(
                    Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
                    Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
                    {imageSize, imageSize}))
                ->fitMode(Hyprtoolkit::IMAGE_FIT_MODE_CONTAIN)
                ->rounding(iconRounding)
                ->sync(false);
            builder->icon(std::get<AppItem>(m_data.data).iconDesc);
            m_image = builder->commence();
            if (m_image) {
                m_image->setPositionFlag(Hyprtoolkit::IElement::HT_POSITION_FLAG_HCENTER, true);
            }
            return;
        }
        if (m_data.isWindow() && std::get<WindowItem>(m_data.data).iconDesc) {
            float baseIconSize = static_cast<float>(m_config.gridItemIconSize);
            float imageSize = hasSub ? (baseIconSize * 0.85f) : baseIconSize;
            int iconRounding = m_config.cornerRadiusSmall;

            auto builder = Hyprtoolkit::CImageBuilder::begin()
                ->size(Hyprtoolkit::CDynamicSize(
                    Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
                    Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
                    {imageSize, imageSize}))
                ->fitMode(Hyprtoolkit::IMAGE_FIT_MODE_CONTAIN)
                ->rounding(iconRounding)
                ->sync(false);
            builder->icon(std::get<WindowItem>(m_data.data).iconDesc);
            m_image = builder->commence();
            if (m_image) {
                m_image->setPositionFlag(Hyprtoolkit::IElement::HT_POSITION_FLAG_HCENTER, true);
            }
            return;
        }
        if (m_data.isWorkspace() && std::get<WorkspaceItem>(m_data.data).iconDesc) {
            float baseIconSize = static_cast<float>(m_config.gridItemIconSize);
            float imageSize = hasSub ? (baseIconSize * 0.85f) : baseIconSize;
            int iconRounding = m_config.cornerRadiusSmall;

            auto builder = Hyprtoolkit::CImageBuilder::begin()
                ->size(Hyprtoolkit::CDynamicSize(
                    Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
                    Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
                    {imageSize, imageSize}))
                ->fitMode(Hyprtoolkit::IMAGE_FIT_MODE_CONTAIN)
                ->rounding(iconRounding)
                ->sync(false);
            builder->icon(std::get<WorkspaceItem>(m_data.data).iconDesc);
            m_image = builder->commence();
            if (m_image) {
                m_image->setPositionFlag(Hyprtoolkit::IElement::HT_POSITION_FLAG_HCENTER, true);
            }
            return;
        }

        std::string iconSource = m_data.iconSource();
        if (iconSource.empty()) {
            m_image = nullptr;
            return;
        }

        float baseIconSize = static_cast<float>(m_config.gridItemIconSize);
        float imageSize = hasSub ? (baseIconSize * 0.85f) : baseIconSize;
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

        m_image = builder->commence();
        if (m_image) {
            m_image->setPositionFlag(Hyprtoolkit::IElement::HT_POSITION_FLAG_HCENTER, true);
        }
    }

    /**
     * @brief Creates the centered title text element.
     */
    void createTitle() {
        std::string titleText = m_data.displayName();
        Hyprtoolkit::CHyprColor col = m_config.gridItemFontColor;
        std::string font = m_config.fontFamily;
        float fontSize = static_cast<float>(m_config.gridItemFontSize);

        m_title = Hyprtoolkit::CTextBuilder::begin()
            ->text(std::move(titleText))
            ->color([col] { return col; })
            ->fontFamily(std::move(font))
            ->align(Hyprtoolkit::HT_FONT_ALIGN_CENTER)
            ->fontSize(Hyprtoolkit::CFontSize(Hyprtoolkit::CFontSize::HT_FONT_ABSOLUTE, fontSize))
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                Hyprtoolkit::CDynamicSize::HT_SIZE_AUTO,
                {1.0f, 0.0f}))
            ->commence();
    }

    /**
     * @brief Creates miniscule subtitle text label for Grid view cards.
     */
    void createSubtitle() {
        std::string sub = m_data.subtitle();
        if (sub.empty()) return;

        Hyprtoolkit::CHyprColor col = m_config.gridItemDescFontColor;
        std::string font = m_config.fontFamily;
        float fontSize = static_cast<float>(m_config.gridItemDescFontSize > 0 ? m_config.gridItemDescFontSize : 9);

        m_subtitle = Hyprtoolkit::CTextBuilder::begin()
            ->text(std::move(sub))
            ->color([col] { return col; })
            ->fontFamily(std::move(font))
            ->align(Hyprtoolkit::HT_FONT_ALIGN_CENTER)
            ->fontSize(Hyprtoolkit::CFontSize(Hyprtoolkit::CFontSize::HT_FONT_ABSOLUTE, fontSize))
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                Hyprtoolkit::CDynamicSize::HT_SIZE_AUTO,
                {1.0f, 0.0f}))
            ->commence();
    }

    /**
     * @brief Attaches mouse click listener for immediate activation.
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
     * @brief Updates tile appearance on selection (hover or keyboard navigation).
     */
    void updateAppearance() {
        Hyprtoolkit::CHyprColor activeBg = m_config.gridItemActiveBackground;
        Hyprtoolkit::CHyprColor inactiveBg = m_config.gridItemBackground;
        Hyprtoolkit::CHyprColor activeBorder = m_config.gridItemActiveBorderColor;
        Hyprtoolkit::CHyprColor inactiveBorder = m_config.gridItemBorderColor;
        int activeBorderSz = m_config.gridItemActiveBorderSize;
        int inactiveBorderSz = m_config.gridItemBorderSize;

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
    CSharedPointer<Hyprtoolkit::CColumnLayoutElement> m_contentLayout;
    CSharedPointer<Hyprtoolkit::CImageElement> m_image;
    CSharedPointer<Hyprtoolkit::CTextElement> m_title;
    CSharedPointer<Hyprtoolkit::CTextElement> m_subtitle;

    Item m_data;
    size_t m_index;
    HLMenu::MenuConfig m_config;
    bool m_selected = false;

    std::function<void(size_t)> m_onActivate;
};

} // namespace HLMenu
