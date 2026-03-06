#pragma once

#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/ScrollArea.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include "Collection.hpp"
#include "ListItem.hpp"
#include <xkbcommon/xkbcommon-keysyms.h>
#include <iostream>
#include <optional>

using namespace Hyprutils::Memory;

class ListView {
public:
    ListView(CSharedPointer<Hyprtoolkit::IBackend> backend)
        : m_backend(backend) {
        
        
        m_scroll = Hyprtoolkit::CScrollAreaBuilder::begin()
            ->scrollY(true)
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                {1.0F, 1.0F}))
            ->commence();
        
        m_layout = Hyprtoolkit::CColumnLayoutBuilder::begin()
            ->gap(2)
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                Hyprtoolkit::CDynamicSize::HT_SIZE_AUTO,
                {1.0F, 1.0F}))
            ->commence();
        m_layout->setMargin(4);
        
        m_scroll->addChild(m_layout);
        
        m_scroll->setRepositioned([this]() { 
            float w = m_scroll->size().x;
            if (std::abs(w - m_lastWidth) > 1.0F) {
                m_lastWidth = w;
            }
        });
    }
    
    CSharedPointer<Hyprtoolkit::IElement> getWidget() { return m_scroll; }
    
    void setCollection(std::shared_ptr<Collection> collection) {
        if (m_collection == collection) return;
        
        m_collection = collection;
        
        // Clear existing items
        m_items.clear();
        m_visibleIndices.clear();
        m_layout->clearChildren();
        
        // Store listener
        m_filterListener = [this]() { updateVisibility(); };
        m_collection->onFilterChange(m_filterListener);
        
        updateVisibility();
    }
    
    void handleKey(const Hyprtoolkit::Input::SKeyboardKeyEvent& e) {
        if (!e.down || !m_collection) return;
        
        if (e.xkbKeysym == XKB_KEY_Up) moveSelection(-1);
        else if (e.xkbKeysym == XKB_KEY_Down) moveSelection(1);
        else if (e.xkbKeysym == XKB_KEY_Page_Up) moveSelection(-10);
        else if (e.xkbKeysym == XKB_KEY_Page_Down) moveSelection(10);
        else if (e.xkbKeysym == XKB_KEY_Home) setSelected(0);
        else if (e.xkbKeysym == XKB_KEY_End) setSelected(m_visibleIndices.size() - 1);
    }
    
    void setOnActivate(std::function<void(int)> cb) { m_onActivate = cb; }
    
    bool hasCollection() const { return m_collection != nullptr; }
    
    void activateCurrentSelection() {
        if (m_onActivate && m_selectedItem < m_items.size()) {
            m_onActivate(m_selectedItem);
        }
    }
    
    std::optional<Item> getSelectedItem() const {
        if (m_selectedItem < m_items.size() && m_items[m_selectedItem]) {
            return m_items[m_selectedItem]->data();
        }
        return std::nullopt;
    }
    
private:
    static constexpr float ITEM_HEIGHT = 50.0f;
    static constexpr int LEFT_PADDING = 10;   
    static constexpr int RIGHT_PADDING = 10; 
    
    void updateVisibility() {
        if (!m_collection) return;
        
        m_visibleIndices = m_collection->getVisibleIndices();
        
        // Create items for visible indices
        for (size_t idx : m_visibleIndices) {
            if (idx >= m_items.size() || !m_items[idx]) {
                createItem(idx);
            }
        }
        
        rebuildList();
        
        if (!m_visibleIndices.empty()) {
            setSelected(0);
        }
    }
    
    void createItem(size_t idx) {
        auto item = std::make_shared<ListItem>(
            m_backend, 
            m_collection->getItem(idx), 
            idx,
            [this](size_t index) {
                if (m_onActivate) m_onActivate(index);
            }
        );
        
        if (idx >= m_items.size()) {
            m_items.resize(idx + 1);
        }
        m_items[idx] = item;
    }
    
    void rebuildList() {
        m_layout->clearChildren();
        
        for (size_t idx : m_visibleIndices) {
            if (idx < m_items.size() && m_items[idx]) {
                m_layout->addChild(m_items[idx]->view());
            }
        }
        
        m_scroll->forceReposition();
    }
    
    void setSelected(size_t visibleIndex) {
        if (visibleIndex >= m_visibleIndices.size()) return;
        
        // Deselect old item
        if (m_selectedItem < m_items.size() && m_items[m_selectedItem]) {
            m_items[m_selectedItem]->setSelected(false);
        }
        
        // Update indices
        m_selected = visibleIndex;
        m_selectedItem = m_visibleIndices[m_selected];
        
        // Select new item
        if (m_selectedItem < m_items.size() && m_items[m_selectedItem]) {
            m_items[m_selectedItem]->setSelected(true);
        }
        
        ensureVisible();
    }
    
    void moveSelection(int delta) {
        if (m_visibleIndices.empty()) return;
        
        int newIdx = m_selected + delta;
        
        if (newIdx < 0) newIdx = m_visibleIndices.size() - 1;
        if (newIdx >= (int)m_visibleIndices.size()) newIdx = 0;
        
        setSelected(newIdx);
    }
    
    void ensureVisible() {
        float scrollY = m_scroll->getCurrentScroll().y;
        float height = m_scroll->size().y;
        float top = m_selected * ITEM_HEIGHT;
        float margin = 4.0f;
        
        if (top < scrollY + margin) {
            m_scroll->setScroll(Hyprutils::Math::Vector2D(0.0, std::max(0.0f, top - margin)));
        } else if (top + ITEM_HEIGHT > scrollY + height - margin) {
            m_scroll->setScroll(Hyprutils::Math::Vector2D(0.0, top + ITEM_HEIGHT - height + margin));
        }
    }
    
    CSharedPointer<Hyprtoolkit::IBackend> m_backend;
    std::shared_ptr<Collection> m_collection;
    CSharedPointer<Hyprtoolkit::CScrollAreaElement> m_scroll;
    CSharedPointer<Hyprtoolkit::CColumnLayoutElement> m_layout;
    
    std::function<void()> m_filterListener;
    std::vector<std::shared_ptr<ListItem>> m_items;
    std::vector<size_t> m_visibleIndices;
    
    size_t m_selected = 0;
    size_t m_selectedItem = 0;
    float m_lastWidth = 0;
    std::function<void(int)> m_onActivate;
};
