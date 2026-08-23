#pragma once

#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/ScrollArea.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include "Core/ItemList.hpp"
#include "GridItem.hpp"
#include "Config/Config.hpp"
#include <xkbcommon/xkbcommon-keysyms.h>
#include <iostream>

using namespace Hyprutils::Memory;

namespace HLMenu {

// ============================================================================
// GRID VIEW - MULTI-COLUMN RESPONSIVE GRID LAYOUT
// ============================================================================
// GridView arranges items into dynamically computed columns inside a smooth,
// kinetically scrolling area.
//
// Key Responsibilities:
//   1. Column count computation based on available container width
//   2. Grid centering by calculating side margin spacers
//   3. Top and bottom margin spacing to prevent borders clipping
//   4. 2D keyboard navigation (Left/Right/Up/Down) with auto-scroll (`ensureVisible`)
//   5. Instant re-rendering when the active ItemList filter updates
// ============================================================================

class GridView {
public:
    /**
     * @brief Constructs the GridView component.
     * @param backend Shared pointer to Hyprtoolkit backend.
     * @param config The active MenuConfig.
     */
    GridView(CSharedPointer<Hyprtoolkit::IBackend> backend, const HLMenu::MenuConfig& config)
        : m_backend(backend), m_config(config) {
        
        // Root vertical scroll area
        m_scroll = Hyprtoolkit::CScrollAreaBuilder::begin()
            ->scrollY(true)
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                {1.0F, 1.0F}))
            ->commence();

        // Inner vertical column layout holding the rows
        m_layout = Hyprtoolkit::CColumnLayoutBuilder::begin()
            ->gap(m_config.gridItemVerticalGap)
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                Hyprtoolkit::CDynamicSize::HT_SIZE_AUTO,
                {1.0F, 1.0F}))
            ->commence();

        m_scroll->addChild(m_layout);

        // Dynamically recompute column layout when window size changes
        m_scroll->setRepositioned([this]() {
            float w = m_scroll->size().x;
            if (std::abs(w - m_lastWidth) > 1.0F) {
                m_lastWidth = w;
                if (m_itemsList) updateGridLayout();
            }
        });
    }

    /// @brief Returns the root scroll widget for embedding into the window layout.
    CSharedPointer<Hyprtoolkit::IElement> getWidget() { return m_scroll; }

    /**
     * @brief Binds an ItemList and registers a filter change listener.
     */
    void setItemList(std::shared_ptr<HLMenu::ItemList> itemsList) {
        if (m_itemsList == itemsList) return;

        m_itemsList = itemsList;

        m_items.clear();
        m_visibleIndices.clear();
        m_layout->clearChildren();

        m_filterListener = [this]() { updateVisibility(); };
        m_itemsList->onFilterChange(m_filterListener);

        updateVisibility();
    }

    /**
     * @brief Clears cached card widgets and refreshes grid for a new dataset.
     */
    void clearItemsCache() {
        m_items.clear();
        m_visibleIndices.clear();
        m_renderedCount = 0;
        m_selected = 0;
        if (m_layout) {
            m_layout->clearChildren();
        }
        updateVisibility();
    }

    /**
     * @brief Processes keyboard navigation events (Arrows, PageUp, PageDown, Home, End).
     */
    void handleKey(const Hyprtoolkit::Input::SKeyboardKeyEvent& e) {
        if (!e.down || !m_itemsList) return;

        if (e.xkbKeysym == XKB_KEY_Left) {
            moveSelection(-1, 0);
        } else if (e.xkbKeysym == XKB_KEY_Right) {
            moveSelection(1, 0);
        } else if (e.xkbKeysym == XKB_KEY_Up) {
            moveSelection(0, -1);
        } else if (e.xkbKeysym == XKB_KEY_Down) {
            moveSelection(0, 1);
        } else if (e.xkbKeysym == XKB_KEY_Page_Up || e.xkbKeysym == XKB_KEY_Prior) {
            float viewH = (m_scroll && m_scroll->size().y > 0) ? m_scroll->size().y : 400.0f;
            int itemStep = std::max(1, m_config.gridItemHeight + m_config.gridItemVerticalGap);
            int rowsPerPage = std::max(1, (int)(viewH / itemStep));
            moveSelection(0, -rowsPerPage);
        } else if (e.xkbKeysym == XKB_KEY_Page_Down || e.xkbKeysym == XKB_KEY_Next) {
            float viewH = (m_scroll && m_scroll->size().y > 0) ? m_scroll->size().y : 400.0f;
            int itemStep = std::max(1, m_config.gridItemHeight + m_config.gridItemVerticalGap);
            int rowsPerPage = std::max(1, (int)(viewH / itemStep));
            moveSelection(0, rowsPerPage);
        } else if (e.xkbKeysym == XKB_KEY_Home) {
            if (!m_visibleIndices.empty()) {
                m_selected = 0;
                updateItemSelection();
                ensureVisible();
            }
        } else if (e.xkbKeysym == XKB_KEY_End) {
            if (!m_visibleIndices.empty()) {
                m_selected = (int)m_visibleIndices.size() - 1;
                updateItemSelection();
                ensureVisible();
            }
        }
    }

    /**
     * @brief Activates the currently highlighted selection.
     */
    void activateCurrentSelection() {
        if (!m_itemsList || m_visibleIndices.empty()) return;
        if (m_selected >= 0 && m_selected < (int)m_visibleIndices.size()) {
            size_t actualIndex = m_visibleIndices[m_selected];
            if (m_onActivate) m_onActivate(actualIndex);
        }
    }

    /**
     * @brief Sets the callback invoked when an item is activated.
     */
    void setOnActivate(std::function<void(size_t)> cb) { m_onActivate = cb; }

    /// @brief Checks if a valid ItemList is bound.
    bool hasItemList() const { return m_itemsList != nullptr; }

private:
    /**
     * @brief Refreshes visible indices from ItemList and rebuilds the grid.
     */
    void updateVisibility() {
        if (!m_itemsList) return;

        m_visibleIndices = m_itemsList->getVisibleIndices();
        m_renderedCount = std::min(m_visibleIndices.size(), (size_t)100);

        createNeededItems();
        updateGridLayout();

        if (!m_visibleIndices.empty()) {
            m_selected = 0;
            updateItemSelection();
            ensureVisible();
        } else {
            m_selected = -1;
        }
    }

    /**
     * @brief Instantiates GridItem elements on-demand for visible entries.
     */
    void createNeededItems() {
        const auto& allItems = m_itemsList->getItems();

        if (m_items.size() < allItems.size()) {
            m_items.resize(allItems.size());
        }

        size_t limit = std::min(m_visibleIndices.size(), m_renderedCount);

        for (size_t i = 0; i < limit; ++i) {
            size_t visibleIdx = m_visibleIndices[i];
            if (!m_items[visibleIdx]) {
                m_items[visibleIdx] = std::make_shared<GridItem>(
                    m_backend,
                    allItems[visibleIdx],
                    visibleIdx,
                    m_config,
                    [this](size_t idx) {
                        if (m_onActivate) m_onActivate(idx);
                    }
                );
            }
        }
    }

    /**
     * @brief Computes maximum column count that fits in the visible width.
     * Clamps to total item count if all items fit in a single row so small menus are centered.
     */
    void updateGridLayout() {
        float availableWidth = m_scroll->size().x;
        if (availableWidth <= 0) {
            availableWidth = static_cast<float>(m_config.windowSize.x - (2 * m_config.windowPadding));
        }

        int itemW = m_config.gridItemWidth;
        int gapX = m_config.gridItemHorizontalGap;

        int maxPossibleColumns = std::max(1, static_cast<int>((availableWidth + gapX) / (itemW + gapX)));

        // If all items fit in a single row, clamp columns to item count for perfect centering
        size_t totalVisible = m_visibleIndices.size();
        if (totalVisible > 0 && totalVisible < static_cast<size_t>(maxPossibleColumns)) {
            m_columns = static_cast<int>(totalVisible);
        } else {
            m_columns = maxPossibleColumns;
        }

        rebuildGrid();
    }

    /**
     * @brief Constructs all rows in the grid layout with side spacers and row gaps.
     */
    void rebuildGrid() {
        m_layout->clearChildren();

        int itemW = m_config.gridItemWidth;
        int itemH = m_config.gridItemHeight;
        int gapX = m_config.gridItemHorizontalGap;

        float totalGridWidth = (m_columns * itemW) + ((m_columns - 1) * gapX);
        float availableWidth = m_scroll->size().x;
        float leftMargin = std::max(0.0F, (availableWidth - totalGridWidth) / 2.0F);

        // Build Grid Rows
        size_t count = std::min(m_visibleIndices.size(), m_renderedCount);
        size_t rows = (count + m_columns - 1) / m_columns;

        for (size_t r = 0; r < rows; ++r) {
            auto row = Hyprtoolkit::CRowLayoutBuilder::begin()
                ->gap(gapX)
                ->size(Hyprtoolkit::CDynamicSize(
                    Hyprtoolkit::CDynamicSize::HT_SIZE_AUTO,
                    Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
                    {totalGridWidth, static_cast<float>(itemH)}))
                ->commence();

            // Left centering spacer
            if (leftMargin > 0) {
                row->addChild(createEmptySpace(leftMargin, itemH));
            }

            for (int c = 0; c < m_columns; ++c) {
                size_t idx = r * m_columns + c;
                if (idx < count) {
                    size_t itemIdx = m_visibleIndices[idx];
                    if (itemIdx < m_items.size() && m_items[itemIdx]) {
                        row->addChild(m_items[itemIdx]->view());
                    }
                } else {
                    // Empty placeholder space to preserve grid alignment
                    row->addChild(createEmptySpace(itemW, itemH));
                }
            }

            // Right centering spacer
            if (leftMargin > 0) {
                row->addChild(createEmptySpace(leftMargin, itemH));
            }

            m_layout->addChild(row);
        }

        // 3. Bottom Spacer
        int bottomPadding = m_config.gridItemVerticalGap;
        if (bottomPadding > 0) {
            auto bottomSpacer = Hyprtoolkit::CRectangleBuilder::begin()
                ->color([] { return Hyprtoolkit::CHyprColor(0, 0, 0, 0); })
                ->size(Hyprtoolkit::CDynamicSize(
                    Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                    Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
                    {1.0F, static_cast<float>(bottomPadding)}))
                ->commence();
            m_layout->addChild(bottomSpacer);
        }

        m_scroll->forceReposition();

        if (m_selected == 0) {
            m_scroll->setScroll(Hyprutils::Math::Vector2D(0.0, 0.0));
        }
    }

    /// @brief Creates an invisible rectangle for centering / layout margins.
    CSharedPointer<Hyprtoolkit::IElement> createEmptySpace(float width, float height) {
        return Hyprtoolkit::CRectangleBuilder::begin()
            ->color([] { return Hyprtoolkit::CHyprColor(0, 0, 0, 0); })
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
                Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
                {width, height}))
            ->commence();
    }

    /**
     * @brief Updates selection cursor coordinates based on arrow direction.
     */
    void moveSelection(int deltaCol, int deltaRow) {
        if (m_visibleIndices.empty() || m_columns <= 0) return;

        int totalItems = (int)m_visibleIndices.size();

        if (m_selected < 0) {
            m_selected = 0;
            updateItemSelection();
            ensureVisible();
            return;
        }

        int currentRow = m_selected / m_columns;
        int currentCol = m_selected % m_columns;

        int newCol = currentCol + deltaCol;
        int newRow = currentRow + deltaRow;

        if (newCol < 0) {
            if (currentRow > 0) {
                newRow--;
                newCol = m_columns - 1;
            } else {
                newCol = 0;
            }
        } else if (newCol >= m_columns) {
            int maxRow = (totalItems - 1) / m_columns;
            if (currentRow < maxRow) {
                newRow++;
                newCol = 0;
            } else {
                newCol = currentCol;
            }
        }

        int maxRow = (totalItems - 1) / m_columns;
        newRow = std::max(0, std::min(newRow, maxRow));

        int newIndex = newRow * m_columns + newCol;

        if (newIndex >= totalItems) {
            newIndex = totalItems - 1;
        }

        if (newIndex >= (int)m_renderedCount && newIndex < totalItems) {
            m_renderedCount = std::min(m_visibleIndices.size(), (size_t)newIndex + 60);
            createNeededItems();
            rebuildGrid();
        }

        if (newIndex != m_selected && newIndex >= 0 && newIndex < totalItems) {
            m_selected = newIndex;
            updateItemSelection();
            ensureVisible();
        }
    }

    /**
     * @brief Updates visual selection highlight on all active tiles.
     */
    void updateItemSelection() {
        size_t count = std::min(m_visibleIndices.size(), m_renderedCount);
        for (size_t i = 0; i < count; ++i) {
            size_t itemIdx = m_visibleIndices[i];
            if (itemIdx < m_items.size() && m_items[itemIdx]) {
                m_items[itemIdx]->setSelected((int)i == m_selected);
            }
        }
    }

    /**
     * @brief Smoothly scrolls the viewport to keep the selected item in full view.
     */
    void ensureVisible() {
        if (m_selected < 0 || m_columns <= 0) return;

        int itemH = m_config.gridItemHeight;
        int gapY = m_config.gridItemVerticalGap;
        int topPadding = m_config.gridItemVerticalGap;
        int bottomPadding = m_config.gridItemVerticalGap;

        int row = m_selected / m_columns;
        float rowTop = topPadding + (row * (itemH + gapY));
        float rowBottom = rowTop + itemH;

        float scrollY = m_scroll->getCurrentScroll().y;
        float viewHeight = m_scroll->size().y;

        if (row == 0 && scrollY > 0) {
            m_scroll->setScroll(Hyprutils::Math::Vector2D(0.0, 0.0));
            return;
        }

        if (rowTop < scrollY) {
            m_scroll->setScroll(Hyprutils::Math::Vector2D(0.0, std::max(0.0f, rowTop - topPadding)));
        } else if (rowBottom + bottomPadding > scrollY + viewHeight) {
            m_scroll->setScroll(Hyprutils::Math::Vector2D(0.0, rowBottom + bottomPadding - viewHeight));
        }
    }

    CSharedPointer<Hyprtoolkit::IBackend> m_backend;
    HLMenu::MenuConfig m_config;
    CSharedPointer<Hyprtoolkit::CScrollAreaElement> m_scroll;
    CSharedPointer<Hyprtoolkit::CColumnLayoutElement> m_layout;
    std::shared_ptr<HLMenu::ItemList> m_itemsList;

    std::vector<std::shared_ptr<GridItem>> m_items;
    std::vector<size_t> m_visibleIndices;
    size_t m_renderedCount = 0;

    int m_selected = -1;
    int m_columns = 1;
    float m_lastWidth = 0;

    std::function<void(size_t)> m_onActivate;
    std::function<void()> m_filterListener;
};

} // namespace HLMenu
