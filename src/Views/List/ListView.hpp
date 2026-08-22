#pragma once

#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/ScrollArea.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include "Core/ItemList.hpp"
#include "ListItem.hpp"
#include "Config/Config.hpp"
#include <xkbcommon/xkbcommon-keysyms.h>
#include <iostream>
#include <optional>

using namespace Hyprutils::Memory;

namespace HLMenu {

// ============================================================================
// LIST VIEW - VERTICAL SCROLLABLE LIST PRESENTATION
// ============================================================================
// ListView displays items as a sleek, vertical single-column list.
//
// Features:
//   1. 1D keyboard navigation (Up/Down/PageUp/PageDown/Home/End)
//   2. Symmetrical top/bottom spacing to prevent edge border clipping
//   3. Auto-scrolling viewport synchronization (`ensureVisible`)
//   4. Instant reactive filtering upon search queries
// ============================================================================

class ListView {
public:
    /**
     * @brief Constructs the ListView component.
     * @param backend Shared pointer to Hyprtoolkit backend.
     * @param config Active MenuConfig.
     */
    ListView(CSharedPointer<Hyprtoolkit::IBackend> backend, const HLMenu::MenuConfig& config)
        : m_backend(backend), m_config(config) {
        
        // Root vertical scroll area
        m_scroll = Hyprtoolkit::CScrollAreaBuilder::begin()
            ->scrollY(true)
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                {1.0F, 1.0F}))
            ->commence();

        // Inner vertical column layout
        m_layout = Hyprtoolkit::CColumnLayoutBuilder::begin()
            ->gap(m_config.listItemsVerticalGap)
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
            }
        });
    }

    /// @brief Returns the root scroll element.
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
     * @brief Clears cached row widgets and refreshes view for a new dataset.
     */
    void clearItemsCache() {
        m_items.clear();
        m_visibleIndices.clear();
        m_renderedCount = 0;
        m_selected = 0;
        m_selectedItem = 0;
        if (m_layout) {
            m_layout->clearChildren();
        }
        updateVisibility();
    }

    /**
     * @brief Handles 1D keyboard navigation (Up, Down, PageUp, PageDown, Home, End).
     */
    void handleKey(const Hyprtoolkit::Input::SKeyboardKeyEvent& e) {
        if (!e.down || !m_itemsList) return;

        if (e.xkbKeysym == XKB_KEY_Up) moveSelection(-1);
        else if (e.xkbKeysym == XKB_KEY_Down) moveSelection(1);
        else if (e.xkbKeysym == XKB_KEY_Page_Up) moveSelection(-10);
        else if (e.xkbKeysym == XKB_KEY_Page_Down) moveSelection(10);
        else if (e.xkbKeysym == XKB_KEY_Home) setSelected(0);
        else if (e.xkbKeysym == XKB_KEY_End) setSelected(m_visibleIndices.size() - 1);
    }

    /**
     * @brief Registers the callback invoked upon row activation.
     */
    void setOnActivate(std::function<void(int)> cb) { m_onActivate = cb; }

    /// @brief Checks if a valid ItemList is bound.
    bool hasItemList() const { return m_itemsList != nullptr; }

    /**
     * @brief Activates the currently highlighted row.
     */
    void activateCurrentSelection() {
        if (m_onActivate && m_selectedItem < m_items.size()) {
            m_onActivate(m_selectedItem);
        }
    }

    /// @brief Returns the currently selected Item if any.
    std::optional<Item> getSelectedItem() const {
        if (m_selectedItem < m_items.size() && m_items[m_selectedItem]) {
            return m_items[m_selectedItem]->data();
        }
        return std::nullopt;
    }

private:
    /**
     * @brief Refreshes visible items based on current search filter.
     */
    void updateVisibility() {
        if (!m_itemsList) return;

        m_visibleIndices = m_itemsList->getVisibleIndices();
        m_renderedCount = std::min(m_visibleIndices.size(), (size_t)100);

        ensureRendered(m_renderedCount);
        rebuildList();

        if (!m_visibleIndices.empty()) {
            setSelected(0);
        }
    }

    /**
     * @brief Ensures item widgets up to count are created.
     */
    void ensureRendered(size_t count) {
        size_t limit = std::min(m_visibleIndices.size(), count);
        for (size_t i = 0; i < limit; ++i) {
            size_t idx = m_visibleIndices[i];
            if (idx >= m_items.size() || !m_items[idx]) {
                createItem(idx);
            }
        }
    }

    /**
     * @brief Instantiates a ListItem on-demand.
     */
    void createItem(size_t idx) {
        auto item = std::make_shared<ListItem>(
            m_backend,
            m_itemsList->getItem(idx),
            idx,
            m_config,
            [this](size_t index) {
                if (m_onActivate) m_onActivate(index);
            }
        );

        if (idx >= m_items.size()) {
            m_items.resize(idx + 1);
        }
        m_items[idx] = item;
    }

    /**
     * @brief Rebuilds the column layout with top/bottom spacers and active rows.
     */
    void rebuildList() {
        m_layout->clearChildren();

        size_t count = std::min(m_visibleIndices.size(), m_renderedCount);

        // 1. Add visible rows
        for (size_t i = 0; i < count; ++i) {
            size_t idx = m_visibleIndices[i];
            if (idx < m_items.size() && m_items[idx]) {
                m_layout->addChild(m_items[idx]->view());
            }
        }

        // 2. Bottom Spacer
        int bottomPadding = m_config.listItemsVerticalGap;
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
    }

    /**
     * @brief Selects an item by visible index.
     */
    void setSelected(size_t visibleIndex) {
        if (visibleIndex >= m_visibleIndices.size()) return;

        if (visibleIndex >= m_renderedCount) {
            m_renderedCount = std::min(m_visibleIndices.size(), visibleIndex + 50);
            ensureRendered(m_renderedCount);
            rebuildList();
        }

        // Deselect previous selection
        if (m_selectedItem < m_items.size() && m_items[m_selectedItem]) {
            m_items[m_selectedItem]->setSelected(false);
        }

        m_selected = visibleIndex;
        m_selectedItem = m_visibleIndices[m_selected];

        // Select new item
        if (m_selectedItem < m_items.size() && m_items[m_selectedItem]) {
            m_items[m_selectedItem]->setSelected(true);
        }

        ensureVisible();
    }

    /**
     * @brief Moves selection cursor up or down by delta steps.
     */
    void moveSelection(int delta) {
        if (m_visibleIndices.empty()) return;

        int newIdx = m_selected + delta;

        if (newIdx < 0) newIdx = m_visibleIndices.size() - 1;
        if (newIdx >= (int)m_visibleIndices.size()) newIdx = 0;

        setSelected(newIdx);
    }

    /**
     * @brief Auto-scrolls viewport to keep active item fully visible.
     */
    void ensureVisible() {
        float itemH = static_cast<float>(m_config.listItemHeight);
        int gapY = m_config.listItemsVerticalGap;
        int topPadding = m_config.listItemsVerticalGap;
        int bottomPadding = m_config.listItemsVerticalGap;

        float scrollY = m_scroll->getCurrentScroll().y;
        float height = m_scroll->size().y;
        float top = topPadding + (m_selected * (itemH + gapY));

        if (top < scrollY + gapY) {
            m_scroll->setScroll(Hyprutils::Math::Vector2D(0.0, std::max(0.0f, top - topPadding)));
        } else if (top + itemH + bottomPadding > scrollY + height) {
            m_scroll->setScroll(Hyprutils::Math::Vector2D(0.0, top + itemH + bottomPadding - height));
        }
    }

    CSharedPointer<Hyprtoolkit::IBackend> m_backend;
    HLMenu::MenuConfig m_config;
    std::shared_ptr<HLMenu::ItemList> m_itemsList;
    CSharedPointer<Hyprtoolkit::CScrollAreaElement> m_scroll;
    CSharedPointer<Hyprtoolkit::CColumnLayoutElement> m_layout;

    std::function<void()> m_filterListener;
    std::vector<std::shared_ptr<ListItem>> m_items;
    std::vector<size_t> m_visibleIndices;
    size_t m_renderedCount = 0;

    size_t m_selected = 0;
    size_t m_selectedItem = 0;
    float m_lastWidth = 0;
    std::function<void(int)> m_onActivate;
};

} // namespace HLMenu
