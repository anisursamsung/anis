#pragma once

#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/Image.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include "Item.hpp"
#include <functional>
#include <string>
#include <filesystem>
#include "SvgConverter.hpp"


#include <algorithm>
#include <cctype>
#include <iostream>

namespace fs = std::filesystem;

using namespace Hyprutils::Memory;

class ListItem {
public:
    ListItem(
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
    const Item& data() const { return m_data; }
    
    void setSelected(bool selected) {
        if (m_selected == selected) return;
        m_selected = selected;
        updateAppearance();
    }
    
private:
    static constexpr float ITEM_HEIGHT = 48.0f;
    static constexpr float ICON_SIZE = 32.0f;
    static constexpr float MAX_TEXT_WIDTH = 400.0f;
    static constexpr int CONTENT_PADDING = 8;
    static constexpr int CONTENT_GAP = 12;
    static constexpr int BORDER_RADIUS = 6;
    static constexpr int BORDER_THICKNESS = 1;
    
    void createUI() {
        auto palette = Hyprtoolkit::CPalette::palette();
        
        m_mainLayout = Hyprtoolkit::CRowLayoutBuilder::begin()
            ->gap(CONTENT_GAP)
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                {1.0f, 1.0f}))
            ->commence();
        m_mainLayout->setMargin(CONTENT_PADDING);
        
        createIcon();
        m_mainLayout->addChild(m_icon);
        
        createTitle();
        m_mainLayout->addChild(m_title);
        
        if (m_data.isFile()) {
            createSubtitle();
            m_mainLayout->addChild(m_subtitle);
        }
        
        auto spacer = Hyprtoolkit::CRectangleBuilder::begin()
            ->color([] { return Hyprtoolkit::CHyprColor(0,0,0,0); })
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                {1.0f, 1.0f}))
            ->commence();
        spacer->setGrow(true, false);
        m_mainLayout->addChild(spacer);
        
        m_background = Hyprtoolkit::CRectangleBuilder::begin()
            ->color([p=palette] { return p->m_colors.base; })
            ->borderColor([p=palette] { 
                auto color = p->m_colors.accent;
                color.a = 0.3f;
                return color; 
            })
            ->borderThickness(BORDER_THICKNESS)
            ->rounding(BORDER_RADIUS)
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
                {1.0f, ITEM_HEIGHT}))
            ->commence();
        
        m_background->addChild(m_mainLayout);
    }
    
//    void createIcon() {
//        auto palette = Hyprtoolkit::CPalette::palette();
//        std::string iconSource = m_data.iconSource();
//        
//        auto builder = Hyprtoolkit::CImageBuilder::begin()
//            ->size(Hyprtoolkit::CDynamicSize(
//                Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
//                Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
//                {ICON_SIZE, ICON_SIZE}))
//            ->fitMode(Hyprtoolkit::IMAGE_FIT_MODE_COVER)
//            ->rounding(palette->m_vars.smallRounding)
//            ->sync(false);
//        
//        if (m_data.isApp() && std::get<AppItem>(m_data.data).iconDesc) {
//            builder->icon(std::get<AppItem>(m_data.data).iconDesc);
//        } else if (!iconSource.empty() && iconSource.find("icon:") != 0) {
//            // Create a copy of the string for path()
//            std::string pathCopy = iconSource;
//            builder->path(std::move(pathCopy));
//        }
//        
//        m_icon = builder->commence();
//    }
//


void createIcon() {
    auto palette = Hyprtoolkit::CPalette::palette();
    
    auto builder = Hyprtoolkit::CImageBuilder::begin()
        ->size(Hyprtoolkit::CDynamicSize(
            Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
            Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
            {ICON_SIZE, ICON_SIZE}))
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
                // Use SvgConverter to handle SVG conversion
                std::string pathToUse = SvgConverter::ensurePngIcon(iconSource, ICON_SIZE);
                std::string pathCopy = pathToUse;
                builder->path(std::move(pathCopy));
                
                //If we don't want to use Svg converter
//                   std::string pathCopy = iconSource;
//                   builder->path(std::move(pathCopy));
                
                
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
    
    m_icon = builder->commence();
}
    
    void createTitle() {
        auto palette = Hyprtoolkit::CPalette::palette();
        
        std::string titleText = m_data.displayName();
        m_title = Hyprtoolkit::CTextBuilder::begin()
            ->text(std::move(titleText))
            ->color([p=palette] { return p->m_colors.text; })
            ->fontFamily(std::string(palette->m_vars.fontFamily))
            ->align(Hyprtoolkit::HT_FONT_ALIGN_LEFT)
            ->fontSize(Hyprtoolkit::CFontSize(Hyprtoolkit::CFontSize::HT_FONT_TEXT))
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_AUTO,
                Hyprtoolkit::CDynamicSize::HT_SIZE_AUTO,
                {0.0f, 0.0f}))
            ->clampSize(Hyprutils::Math::Vector2D(MAX_TEXT_WIDTH, 0.0f))
            ->commence();
    }
    
    void createSubtitle() {
        auto palette = Hyprtoolkit::CPalette::palette();
        
        if (!m_data.isFile()) return;
        
        const auto& file = std::get<FileItem>(m_data.data);
        std::string path = file.filePath.filename().string();
        
        m_subtitle = Hyprtoolkit::CTextBuilder::begin()
            ->text(std::move(path))
            ->color([p=palette] { 
                auto color = p->m_colors.text;
                color.a = 0.7f;
                return color; 
            })
            ->fontFamily(std::string(palette->m_vars.fontFamily))
            ->align(Hyprtoolkit::HT_FONT_ALIGN_LEFT)
            ->fontSize(Hyprtoolkit::CFontSize(Hyprtoolkit::CFontSize::HT_FONT_SMALL))
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_AUTO,
                Hyprtoolkit::CDynamicSize::HT_SIZE_AUTO,
                {0.0f, 0.0f}))
            ->clampSize(Hyprutils::Math::Vector2D(MAX_TEXT_WIDTH, 0.0f))
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
                builder->color([=] { return palette->m_colors.base; });
                auto borderColor = palette->m_colors.accent;
                borderColor.a = 0.3f;
                builder->borderColor([=] { return borderColor; });
            }
            builder->commence();
        }
        
        m_mainLayout->removeChild(m_title);
        
        std::string titleText = m_data.displayName();
        if (m_selected) {
            m_title = Hyprtoolkit::CTextBuilder::begin()
                ->text(std::move(titleText))
                ->color([=] { return palette->m_colors.brightText; })
                ->fontFamily(std::string(palette->m_vars.fontFamily))
                ->align(Hyprtoolkit::HT_FONT_ALIGN_LEFT)
                ->fontSize(Hyprtoolkit::CFontSize(Hyprtoolkit::CFontSize::HT_FONT_TEXT))
                ->size(Hyprtoolkit::CDynamicSize(
                    Hyprtoolkit::CDynamicSize::HT_SIZE_AUTO,
                    Hyprtoolkit::CDynamicSize::HT_SIZE_AUTO,
                    {0.0f, 0.0f}))
                ->clampSize(Hyprutils::Math::Vector2D(MAX_TEXT_WIDTH, 0.0f))
                ->commence();
        } else {
            m_title = Hyprtoolkit::CTextBuilder::begin()
                ->text(std::move(titleText))
                ->color([=] { return palette->m_colors.text; })
                ->fontFamily(std::string(palette->m_vars.fontFamily))
                ->align(Hyprtoolkit::HT_FONT_ALIGN_LEFT)
                ->fontSize(Hyprtoolkit::CFontSize(Hyprtoolkit::CFontSize::HT_FONT_TEXT))
                ->size(Hyprtoolkit::CDynamicSize(
                    Hyprtoolkit::CDynamicSize::HT_SIZE_AUTO,
                    Hyprtoolkit::CDynamicSize::HT_SIZE_AUTO,
                    {0.0f, 0.0f}))
                ->clampSize(Hyprutils::Math::Vector2D(MAX_TEXT_WIDTH, 0.0f))
                ->commence();
        }
        
        m_mainLayout->addChild(m_title);
        
        if (m_subtitle) {
            m_mainLayout->removeChild(m_subtitle);
            
            if (m_data.isFile()) {
                const auto& file = std::get<FileItem>(m_data.data);
                std::string path = file.filePath.filename().string();
                
                auto subtitleColor = m_selected ? 
                    palette->m_colors.brightText : 
                    palette->m_colors.text.mix(palette->m_colors.brightText, 0.7f);
                
                m_subtitle = Hyprtoolkit::CTextBuilder::begin()
                    ->text(std::move(path))
                    ->color([=] { return subtitleColor; })
                    ->fontFamily(std::string(palette->m_vars.fontFamily))
                    ->align(Hyprtoolkit::HT_FONT_ALIGN_LEFT)
                    ->fontSize(Hyprtoolkit::CFontSize(Hyprtoolkit::CFontSize::HT_FONT_SMALL))
                    ->size(Hyprtoolkit::CDynamicSize(
                        Hyprtoolkit::CDynamicSize::HT_SIZE_AUTO,
                        Hyprtoolkit::CDynamicSize::HT_SIZE_AUTO,
                        {0.0f, 0.0f}))
                    ->clampSize(Hyprutils::Math::Vector2D(MAX_TEXT_WIDTH, 0.0f))
                    ->commence();
                
                m_mainLayout->addChild(m_subtitle);
            }
        }
        
        m_mainLayout->forceReposition();
    }
    
    CSharedPointer<Hyprtoolkit::IBackend> m_backend;
    CSharedPointer<Hyprtoolkit::CRectangleElement> m_background;
    CSharedPointer<Hyprtoolkit::CRowLayoutElement> m_mainLayout;
    CSharedPointer<Hyprtoolkit::CImageElement> m_icon;
    CSharedPointer<Hyprtoolkit::CTextElement> m_title;
    CSharedPointer<Hyprtoolkit::CTextElement> m_subtitle;
    
    Item m_data;
    size_t m_index;
    bool m_selected = false;
    
    std::function<void(size_t)> m_onActivate;
};
