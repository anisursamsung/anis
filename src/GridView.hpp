#pragma once

#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/ScrollArea.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include "Collection.hpp"
#include "GridItem.hpp"
#include <xkbcommon/xkbcommon-keysyms.h>
#include <iostream>

using namespace Hyprutils::Memory;

class GridView {
public:
    GridView(CSharedPointer<Hyprtoolkit::IBackend> backend)
        : m_backend(backend) {
        
        
        m_scroll = Hyprtoolkit::CScrollAreaBuilder::begin()
            ->scrollY(true)
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                {1.0F, 1.0F}))
            ->commence();
        
        m_layout = Hyprtoolkit::CColumnLayoutBuilder::begin()
            ->gap(12)
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                Hyprtoolkit::CDynamicSize::HT_SIZE_AUTO,
                {1.0F, 1.0F}))
            ->commence();
        
        m_scroll->addChild(m_layout);
        
        m_scroll->setRepositioned([this]() { 
            float w = m_scroll->size().x;
            if (std::abs(w - m_lastWidth) > 1.0F) {
                m_lastWidth = w;
                if (m_collection) updateGridLayout();
            }
        });
    }
    
    CSharedPointer<Hyprtoolkit::IElement> getWidget() { return m_scroll; }
    
    void setCollection(std::shared_ptr<Collection> collection) {
        if (m_collection == collection) return;
        
        m_collection = collection;
        
        m_items.clear();
        m_visibleIndices.clear();
        m_layout->clearChildren();
        
        m_filterListener = [this]() { updateVisibility(); };
        m_collection->onFilterChange(m_filterListener);
        
        updateVisibility();
    }
    
    void handleKey(const Hyprtoolkit::Input::SKeyboardKeyEvent& e) {
        if (!e.down || !m_collection) return;
        
        if (e.xkbKeysym == XKB_KEY_Left) moveSelection(-1, 0);
        else if (e.xkbKeysym == XKB_KEY_Right) moveSelection(1, 0);
        else if (e.xkbKeysym == XKB_KEY_Up) moveSelection(0, -1);
        else if (e.xkbKeysym == XKB_KEY_Down) moveSelection(0, 1);
    }
    
    void setOnActivate(std::function<void(int)> cb) { m_onActivate = cb; }
    
    bool hasCollection() const { return m_collection != nullptr; }
    
    void activateCurrentSelection() {
        if (m_onActivate && m_selectedItem < m_items.size()) {
            m_onActivate(m_selectedItem);
        }
    }
    
private:
    static constexpr int ITEM_SIZE = 140;
    static constexpr int HORIZONTAL_SPACING = 20;
    static constexpr int VERTICAL_SPACING = 20;
    static constexpr int LEFT_PADDING = 10;
    static constexpr int RIGHT_PADDING = 10;
    static constexpr int TOP_PADDING = 10;
    static constexpr int BOTTOM_PADDING = 10;
    
    void updateVisibility() {
        if (!m_collection) return;
        
        m_visibleIndices = m_collection->getVisibleIndices();
        
        for (size_t idx : m_visibleIndices) {
            if (idx >= m_items.size() || !m_items[idx]) {
                createItem(idx);
            }
        }
        
        updateGridLayout();
        
        if (!m_visibleIndices.empty()) {
            setSelected(0);
        }
    }
    
    void createItem(size_t idx) {
        auto item = std::make_shared<GridItem>(
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
    
    void updateGridLayout() {
        if (!m_collection || m_visibleIndices.empty()) return;
        
        float availableWidth = m_scroll->size().x;
        if (availableWidth < 10.0F) availableWidth = 800.0F;
        
        float contentWidth = availableWidth - LEFT_PADDING - RIGHT_PADDING;
        float totalWidthPerColumn = ITEM_SIZE + HORIZONTAL_SPACING;
        int maxColumns = static_cast<int>((contentWidth + HORIZONTAL_SPACING) / totalWidthPerColumn);
        
        int totalVisible = static_cast<int>(m_visibleIndices.size());
        m_columns = std::max(1, std::min(maxColumns, totalVisible));
        m_columns = std::min(m_columns, 8);
        
        rebuildGrid();
    }
    
    void rebuildGrid() {
        m_layout->clearChildren();
        
        // Add top padding spacer
        if (TOP_PADDING > 0) {
            auto topSpacer = Hyprtoolkit::CRectangleBuilder::begin()
                ->color([] { return Hyprtoolkit::CHyprColor(0,0,0,0); })
                ->size(Hyprtoolkit::CDynamicSize(
                    Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                    Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
                    {1.0F, (float)TOP_PADDING}))
                ->commence();
            m_layout->addChild(topSpacer);
        }
        
        float totalGridWidth = (m_columns * ITEM_SIZE) + ((m_columns - 1) * HORIZONTAL_SPACING);
        float availableWidth = m_scroll->size().x;
        float leftMargin = std::max(0.0F, (availableWidth - totalGridWidth - LEFT_PADDING - RIGHT_PADDING) / 2.0F);
        
        size_t rows = (m_visibleIndices.size() + m_columns - 1) / m_columns;
        
        CSharedPointer<Hyprtoolkit::CColumnLayoutElement> gridWrapper;
        if (leftMargin > 0) {
            gridWrapper = Hyprtoolkit::CColumnLayoutBuilder::begin()
                ->gap(VERTICAL_SPACING)
                ->size(Hyprtoolkit::CDynamicSize(
                    Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                    Hyprtoolkit::CDynamicSize::HT_SIZE_AUTO,
                    {1.0F, 1.0F}))
                ->commence();
            gridWrapper->setPositionMode(Hyprtoolkit::IElement::HT_POSITION_AUTO);
            gridWrapper->setPositionFlag(Hyprtoolkit::IElement::HT_POSITION_FLAG_HCENTER, true);
        }
        
        auto& container = gridWrapper ? gridWrapper : m_layout;
        
        for (size_t r = 0; r < rows; ++r) {
            auto row = Hyprtoolkit::CRowLayoutBuilder::begin()
                ->gap(HORIZONTAL_SPACING)
                ->size(Hyprtoolkit::CDynamicSize(
                    Hyprtoolkit::CDynamicSize::HT_SIZE_AUTO,
                    Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
                    {totalGridWidth, (float)ITEM_SIZE}))
                ->commence();
            
            if (leftMargin > 0) {
                row->addChild(createEmptySpace(leftMargin, ITEM_SIZE));
            }
            
            for (int c = 0; c < m_columns; ++c) {
                size_t idx = r * m_columns + c;
                if (idx < m_visibleIndices.size()) {
                    size_t itemIdx = m_visibleIndices[idx];
                    if (itemIdx < m_items.size() && m_items[itemIdx]) {
                        row->addChild(m_items[itemIdx]->view());
                    }
                } else {
                    row->addChild(createEmptySpace(ITEM_SIZE, ITEM_SIZE));
                }
            }
            
            if (leftMargin > 0) {
                row->addChild(createEmptySpace(leftMargin, ITEM_SIZE));
            }
            
            container->addChild(row);
        }
        
        if (gridWrapper) {
            m_layout->addChild(gridWrapper);
        }
        
        // Add bottom padding spacer
        if (BOTTOM_PADDING > 0) {
            auto bottomSpacer = Hyprtoolkit::CRectangleBuilder::begin()
                ->color([] { return Hyprtoolkit::CHyprColor(0,0,0,0); })
                ->size(Hyprtoolkit::CDynamicSize(
                    Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                    Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
                    {1.0F, (float)BOTTOM_PADDING}))
                ->commence();
            m_layout->addChild(bottomSpacer);
        }
        
        m_scroll->forceReposition();
        
        // Force scroll to show the top content (after the spacer)
        if (m_selected == 0) {
            m_scroll->setScroll(Hyprutils::Math::Vector2D(0.0, 0.0));
        }
    }
    
    CSharedPointer<Hyprtoolkit::IElement> createEmptySpace(float width, float height) {
        return Hyprtoolkit::CRectangleBuilder::begin()
            ->color([] { return Hyprtoolkit::CHyprColor(0,0,0,0); })
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
                Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
                {width, height}))
            ->commence();
    }
    
    void setSelected(size_t visibleIndex) {
        if (visibleIndex >= m_visibleIndices.size()) return;
        
        if (m_selectedItem < m_items.size() && m_items[m_selectedItem]) {
            m_items[m_selectedItem]->setSelected(false);
        }
        
        m_selected = visibleIndex;
        m_selectedItem = m_visibleIndices[m_selected];
        
        if (m_selectedItem < m_items.size() && m_items[m_selectedItem]) {
            m_items[m_selectedItem]->setSelected(true);
        }
        
        ensureVisible();
    }
    
    void moveSelection(int dc, int dr) {
        if (m_visibleIndices.empty()) return;
        
        int row = m_selected / m_columns;
        int col = m_selected % m_columns;
        
        col += dc;
        row += dr;
        
        if (col < 0) { col = m_columns - 1; row--; }
        if (col >= m_columns) { col = 0; row++; }
        if (row < 0) row = (m_visibleIndices.size() - 1) / m_columns;
        if (row >= (int)((m_visibleIndices.size() + m_columns - 1) / m_columns)) row = 0;
        
        int newIdx = row * m_columns + col;
        if (newIdx >= (int)m_visibleIndices.size()) newIdx = m_visibleIndices.size() - 1;
        
        setSelected(newIdx);
    }
    
    void ensureVisible() {
        if (m_visibleIndices.empty()) return;
        
        int row = m_selected / m_columns;
        
        // The first row's top position is TOP_PADDING (from the spacer)
        float rowTop = TOP_PADDING + (row * (ITEM_SIZE + VERTICAL_SPACING));
        float rowBottom = rowTop + ITEM_SIZE;
        
        float scrollY = m_scroll->getCurrentScroll().y;
        float viewHeight = m_scroll->size().y;
        
        // If this is the first row, ensure we see it fully
        if (row == 0) {
            if (scrollY > 0) {
                m_scroll->setScroll(Hyprutils::Math::Vector2D(0.0, 0.0));
            }
            return;
        }
        
        // Check if row is above visible area
        if (rowTop < scrollY) {
            m_scroll->setScroll(Hyprutils::Math::Vector2D(0.0, rowTop - VERTICAL_SPACING));
        }
        // Check if row is below visible area
        else if (rowBottom > scrollY + viewHeight) {
            m_scroll->setScroll(Hyprutils::Math::Vector2D(0.0, rowBottom - viewHeight + VERTICAL_SPACING));
        }
    }
    
    CSharedPointer<Hyprtoolkit::IBackend> m_backend;
    std::shared_ptr<Collection> m_collection;
    CSharedPointer<Hyprtoolkit::CScrollAreaElement> m_scroll;
    CSharedPointer<Hyprtoolkit::CColumnLayoutElement> m_layout;
    
    std::function<void()> m_filterListener;
    std::vector<std::shared_ptr<GridItem>> m_items;
    std::vector<size_t> m_visibleIndices;
    
    size_t m_selected = 0;
    size_t m_selectedItem = 0;
    int m_columns = 4;
    float m_lastWidth = 0;
    std::function<void(int)> m_onActivate;
};
