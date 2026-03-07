#pragma once

#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/Image.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include "Item.hpp"
#include "SvgConverter.hpp" 
#include <functional>
#include <string>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <iostream>

namespace fs = std::filesystem;

using namespace Hyprutils::Memory;

class GridItem {
public:
    GridItem(
        CSharedPointer<Hyprtoolkit::IBackend> backend,
        const Item& data,
        size_t index,
        std::function<void(size_t)> onActivate
    ) : m_backend(backend)
      , m_data(data)
      , m_index(index)
      , m_onActivate(onActivate) {
        
        createUI();
        setupMouseHandlers();
    }
    
    CSharedPointer<Hyprtoolkit::IElement> view() const { return m_background; }
    size_t index() const { return m_index; }
    
    void setSelected(bool selected) {
        if (m_selected == selected) return;
        m_selected = selected;
        updateAppearance();
    }
    
private:
    static constexpr float ITEM_WIDTH = 140.0f;
    static constexpr float ITEM_HEIGHT = 160.0f;
    static constexpr float IMAGE_HEIGHT_RATIO = 0.8f;  
    static constexpr float TEXT_HEIGHT_RATIO = 0.2f;   
    static constexpr float IMAGE_PADDING_VERTICAL = 50.0f;
    static constexpr float TEXT_PADDING_HORIZONTAL = 5.0f;
    static constexpr float TEXT_PADDING_VERTICAL = 15.0f;
    static constexpr int CONTENT_PADDING = 10;
    static constexpr int CONTENT_GAP = 10;
    static constexpr int BORDER_RADIUS = 10;
    static constexpr int BORDER_THICKNESS = 0;
    
    float m_cachedImageHeight = 0.0f;
    float m_cachedTextHeight = 0.0f;
    
    void createUI() {
        auto palette = Hyprtoolkit::CPalette::palette();
        
        m_cachedImageHeight = ITEM_HEIGHT * IMAGE_HEIGHT_RATIO;
        m_cachedTextHeight = ITEM_HEIGHT * TEXT_HEIGHT_RATIO;
        
        m_mainLayout = Hyprtoolkit::CColumnLayoutBuilder::begin()
            ->gap(0)
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
                Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
                {ITEM_WIDTH - (BORDER_THICKNESS * 2), 
                 ITEM_HEIGHT - (BORDER_THICKNESS * 2)}))
            ->commence();
        
        m_contentLayout = Hyprtoolkit::CColumnLayoutBuilder::begin()
            ->gap(CONTENT_GAP)
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                {1.0f, 1.0f}))
            ->commence();
        m_contentLayout->setMargin(CONTENT_PADDING);
        
        createImage();
        m_contentLayout->addChild(m_image);
        
        createTitle();
        m_contentLayout->addChild(m_title);
        
        m_mainLayout->addChild(m_contentLayout);
        
        m_background = Hyprtoolkit::CRectangleBuilder::begin()
            ->color([p=palette] { return p->m_colors.alternateBase; })
            ->borderColor([p=palette] { 
                auto color = p->m_colors.accent;
                color.a = 0.3f;
                return color; 
            })
            ->borderThickness(BORDER_THICKNESS)
            ->rounding(BORDER_RADIUS)
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
                Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
                {ITEM_WIDTH, ITEM_HEIGHT}))
            ->commence();
        
        m_background->addChild(m_mainLayout);
    }

    void createImage() {
        auto palette = Hyprtoolkit::CPalette::palette();
        
        float imageSize = m_cachedImageHeight - IMAGE_PADDING_VERTICAL;
        
        auto builder = Hyprtoolkit::CImageBuilder::begin()
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
                Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
                {imageSize, imageSize}))
            ->fitMode(Hyprtoolkit::IMAGE_FIT_MODE_COVER)
            ->rounding(palette->m_vars.smallRounding)
            ->sync(false);
        
        // PATHWAY 1: Launcher mode with system icon (DIRECT)
        if (m_data.isApp() && std::get<AppItem>(m_data.data).iconDesc) {
            builder->icon(std::get<AppItem>(m_data.data).iconDesc);
        }
        // PATHWAY 2: Everything else
        else {
            std::string iconSource = m_data.iconSource();
            
            if (!iconSource.empty() && iconSource.find("icon:") != 0) {
                // Try as file path first
                if (fs::exists(iconSource)) {
                    // Svg converter
                    std::string pathToUse = SvgConverter::ensurePngIcon(iconSource, imageSize);
                    std::string pathCopy = pathToUse;
                    builder->path(std::move(pathCopy));
                    
                    
                    //if we don't want to use Svgconverter
//                    std::string pathCopy = iconSource;
//                    builder->path(std::move(pathCopy));
                    
                    
                    
                    
                } 
                // Not a file, try as system icon name
                else {
                    auto iconFactory = m_backend->systemIcons();
                    if (iconFactory) {
                        auto iconDesc = iconFactory->lookupIcon(iconSource);
                        if (iconDesc && iconDesc->exists()) {
                            builder->icon(iconDesc);
                        } else {
                            auto fallback = iconFactory->lookupIcon("image-missing");
                            if (fallback && fallback->exists()) {
                                builder->icon(fallback);
                            }
                        }
                    }
                }
            }
        }
        
        m_image = builder->commence();
    }
    
    void createTitle() {
        auto palette = Hyprtoolkit::CPalette::palette();
        
        std::string titleText = m_data.displayName();
        m_title = Hyprtoolkit::CTextBuilder::begin()
            ->text(std::move(titleText))
            ->color([p=palette] { return p->m_colors.text; })
            ->fontFamily(std::string(palette->m_vars.fontFamily))
            ->align(Hyprtoolkit::HT_FONT_ALIGN_CENTER)
            ->fontSize(Hyprtoolkit::CFontSize(Hyprtoolkit::CFontSize::HT_FONT_SMALL))
            ->clampSize(Hyprutils::Math::Vector2D(
                ITEM_WIDTH - TEXT_PADDING_HORIZONTAL, 
                m_cachedTextHeight - TEXT_PADDING_VERTICAL))
            ->commence();
    }
    
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
    
    void updateAppearance() {
        auto palette = Hyprtoolkit::CPalette::palette();
        
        if (auto builder = m_background->rebuild()) {
            if (m_selected) {
                builder->color([=] { return palette->m_colors.accent; });
                builder->borderColor([=] { return palette->m_colors.accent; });
            } else {
                builder->color([=] { return palette->m_colors.alternateBase; });
                auto borderColor = palette->m_colors.accent;
                borderColor.a = 0.3f;
                builder->borderColor([=] { return borderColor; });
            }
            builder->commence();
        }
        
        m_contentLayout->removeChild(m_title);
        
        std::string titleText = m_data.displayName();
        if (m_selected) {
            m_title = Hyprtoolkit::CTextBuilder::begin()
                ->text(std::move(titleText))
                ->color([=] { return palette->m_colors.brightText; })
                ->fontFamily(std::string(palette->m_vars.fontFamily))
                ->align(Hyprtoolkit::HT_FONT_ALIGN_CENTER)
                ->fontSize(Hyprtoolkit::CFontSize(Hyprtoolkit::CFontSize::HT_FONT_SMALL))
                ->clampSize(Hyprutils::Math::Vector2D(
                    ITEM_WIDTH - TEXT_PADDING_HORIZONTAL, 
                    m_cachedTextHeight - TEXT_PADDING_VERTICAL))
                ->commence();
        } else {
            m_title = Hyprtoolkit::CTextBuilder::begin()
                ->text(std::move(titleText))
                ->color([=] { return palette->m_colors.text; })
                ->fontFamily(std::string(palette->m_vars.fontFamily))
                ->align(Hyprtoolkit::HT_FONT_ALIGN_CENTER)
                ->fontSize(Hyprtoolkit::CFontSize(Hyprtoolkit::CFontSize::HT_FONT_SMALL))
                ->clampSize(Hyprutils::Math::Vector2D(
                    ITEM_WIDTH - TEXT_PADDING_HORIZONTAL, 
                    m_cachedTextHeight - TEXT_PADDING_VERTICAL))
                ->commence();
        }
        
        m_contentLayout->addChild(m_title);
        m_contentLayout->forceReposition();
    }
    
    CSharedPointer<Hyprtoolkit::IBackend> m_backend;
    CSharedPointer<Hyprtoolkit::CRectangleElement> m_background;
    CSharedPointer<Hyprtoolkit::CColumnLayoutElement> m_mainLayout;
    CSharedPointer<Hyprtoolkit::CColumnLayoutElement> m_contentLayout;
    CSharedPointer<Hyprtoolkit::CImageElement> m_image;
    CSharedPointer<Hyprtoolkit::CTextElement> m_title;
    
    Item m_data;
    size_t m_index;
    bool m_selected = false;
    
    std::function<void(size_t)> m_onActivate;
};
