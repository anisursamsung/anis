#pragma once

#include "Item.hpp"
#include <vector>
#include <string>
#include <functional>
#include <algorithm>
#include <cctype>
#include <iostream>

namespace HLMenu {

// ============================================================================
// ITEMLIST - REACTIVE FILTERABLE ITEM CONTAINER
// ============================================================================
// ItemList stores the complete collection of Items for hlmenu.
// It manages:
//   1. Full item dataset (`m_allItems`)
//   2. Real-time substring filter query (`m_filterQuery`)
//   3. Indices of visible items that match the current query (`m_visibleIndices`)
//   4. Observer callbacks notifying views (GridView / ListView) whenever the
//      filter query updates so the layout can refresh instantly.
// ============================================================================

class ItemList {
public:
    /**
     * @brief Replace all items in the list and recompute the filter.
     * @param items The new list of items (e.g. from AppProvider or FileProvider).
     */
    void setItems(const std::vector<Item>& items) {
        m_allItems = items;
        applyFilter();
        notifyFilterChanged();
    }

    /**
     * @brief Apply a new search query to filter items.
     * Triggers all registered `onFilterChange` listeners if query changed.
     * @param filter The search query text.
     */
    void setFilter(const std::string& filter) {
        if (m_filterQuery == filter) return;

        m_filterQuery = filter;
        applyFilter();
        notifyFilterChanged();
    }

    /// @brief Returns the current search filter string.
    const std::string& getFilter() const { return m_filterQuery; }

    /// @brief Returns the total count of items before filtering.
    size_t totalCount() const { return m_allItems.size(); }

    /// @brief Returns the count of items that match the current search query.
    size_t visibleCount() const { return m_visibleIndices.size(); }

    /// @brief Retrieves an item by its raw master index.
    const Item& getItem(size_t index) const { return m_allItems[index]; }

    /// @brief Returns a reference to all raw items.
    const std::vector<Item>& getItems() const { return m_allItems; }

    /// @brief Checks if a specific master item index is currently visible.
    bool isItemVisible(size_t index) const {
        return std::find(m_visibleIndices.begin(), m_visibleIndices.end(), index) != m_visibleIndices.end();
    }

    /// @brief Returns the vector of item indices that match the active search query.
    const std::vector<size_t>& getVisibleIndices() const {
        return m_visibleIndices;
    }

    /**
     * @brief Register a callback to be notified when the filter query updates.
     * Used by GridView and ListView to rebuild visible rows on keypress.
     */
    void onFilterChange(std::function<void()> cb) {
        m_onFilterChange.push_back(cb);
    }

private:
    /**
     * @brief Recomputes `m_visibleIndices` based on zero-allocation case-insensitive matching.
     */
    void applyFilter() {
        m_visibleIndices.clear();

        // If query is empty, all items are visible
        if (m_filterQuery.empty()) {
            m_visibleIndices.reserve(m_allItems.size());
            for (size_t i = 0; i < m_allItems.size(); ++i) {
                m_visibleIndices.push_back(i);
            }
            return;
        }

        std::string_view query = m_filterQuery;

        auto caseInsensitiveFind = [](std::string_view haystack, std::string_view needle) -> bool {
            if (needle.empty()) return true;
            if (haystack.length() < needle.length()) return false;
            auto it = std::search(
                haystack.begin(), haystack.end(),
                needle.begin(), needle.end(),
                [](char ch1, char ch2) {
                    return std::tolower(static_cast<unsigned char>(ch1)) ==
                           std::tolower(static_cast<unsigned char>(ch2));
                }
            );
            return it != haystack.end();
        };

        m_visibleIndices.reserve(std::min(m_allItems.size(), (size_t)100));

        for (size_t i = 0; i < m_allItems.size(); ++i) {
            const auto& item = m_allItems[i];
            if (caseInsensitiveFind(item.displayName(), query) || caseInsensitiveFind(item.subtitle(), query)) {
                m_visibleIndices.push_back(i);
            }
        }
    }

    /**
     * @brief Invokes all registered observer callbacks.
     */
    void notifyFilterChanged() {
        for (auto& cb : m_onFilterChange) {
            cb();
        }
    }

    std::vector<Item> m_allItems;                          ///< Complete master dataset
    std::vector<size_t> m_visibleIndices;                  ///< Filtered item indices
    std::string m_filterQuery;                             ///< Active search query string
    std::vector<std::function<void()>> m_onFilterChange;   ///< Filter change listeners
};

} // namespace HLMenu
