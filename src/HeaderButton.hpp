#pragma once

#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/Image.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <hyprutils/memory/SharedPtr.hpp>
#include <functional>
#include <string>

using namespace Hyprutils::Memory;

class HeaderButton {
public:
    // Constructor for system icon
    HeaderButton(
        CSharedPointer<Hyprtoolkit::IBackend> backend,
        const std::string& iconName,
        int size = 42
    ) : m_backend(backend), m_iconName(iconName), m_size(size) {
        
        createUI();
        setupMouseHandlers();
    }
    
    CSharedPointer<Hyprtoolkit::IElement> getView() const { return m_rootElement; }
    
    void onClick(std::function<void()> callback) {
        m_onClick = callback;
    }
    
private:
    void createUI() {
        auto palette = Hyprtoolkit::CPalette::palette();
        
        // Create icon first
        createIcon();
        
        // Background with icon directly added
        m_rootElement = Hyprtoolkit::CRectangleBuilder::begin()
            ->color([this] { return getCurrentBackgroundColor(); })
            ->borderColor([this] { return getCurrentBorderColor(); })
            ->borderThickness(1)
            ->rounding(m_size / 4)
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
                Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
                {(float)m_size, (float)m_size}))
            ->commence();
        
        // Add icon directly to background
        m_rootElement->addChild(m_icon);
    }
    
    void createIcon() {
        auto iconBuilder = Hyprtoolkit::CImageBuilder::begin()
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
                Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
                {(float)(m_size * 0.8f), (float)(m_size * 0.8f)}))  // Slightly larger
            ->fitMode(Hyprtoolkit::IMAGE_FIT_MODE_CONTAIN)
            ->rounding(4)
            ->sync(false);
        
        // Try to load as system icon first
        auto iconFactory = m_backend->systemIcons();
        if (iconFactory) {
            auto iconDesc = iconFactory->lookupIcon(m_iconName);
            if (iconDesc && iconDesc->exists()) {
                iconBuilder->icon(iconDesc);
            } else {
                // Fallback to path
                std::string pathCopy = m_iconName;
                iconBuilder->path(std::move(pathCopy));
            }
        } else {
            // No icon factory, use path
            std::string pathCopy = m_iconName;
            iconBuilder->path(std::move(pathCopy));
        }
        
        m_icon = iconBuilder->commence();
    }
    
    void setupMouseHandlers() {
        m_rootElement->setReceivesMouse(true);
        
        m_rootElement->setMouseEnter([this](const auto&) {
            m_hovered = true;
            updateAppearance();
        });
        
        m_rootElement->setMouseLeave([this]() {
            m_hovered = false;
            updateAppearance();
        });
        
        m_rootElement->setMouseButton([this](auto button, bool down) {
            if (down && button == Hyprtoolkit::Input::MOUSE_BUTTON_LEFT && m_onClick) {
                m_onClick();
            }
        });
    }
    
    Hyprtoolkit::CHyprColor getCurrentBackgroundColor() const {
        auto palette = Hyprtoolkit::CPalette::palette();
        if (m_hovered) {
            auto color = palette->m_colors.accent;
            color.a = 0.2f;
            return color;
        }
        return palette->m_colors.alternateBase;
    }
    
    Hyprtoolkit::CHyprColor getCurrentBorderColor() const {
        auto palette = Hyprtoolkit::CPalette::palette();
        if (m_hovered) {
            return palette->m_colors.accent;
        }
        auto color = palette->m_colors.accent;
        color.a = 0.3f;
        return color;
    }
    
    void updateAppearance() {
        if (auto builder = m_rootElement->rebuild()) {
            builder
                ->color([this] { return getCurrentBackgroundColor(); })
                ->borderColor([this] { return getCurrentBorderColor(); })
                ->commence();
        }
    }
    
    CSharedPointer<Hyprtoolkit::IBackend> m_backend;
    CSharedPointer<Hyprtoolkit::CRectangleElement> m_rootElement;
    CSharedPointer<Hyprtoolkit::CImageElement> m_icon;
    
    std::string m_iconName;
    int m_size;
    
    bool m_hovered = false;
    std::function<void()> m_onClick;
};
