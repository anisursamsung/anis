#pragma once

#include "Item.hpp"
#include <vector>
#include <string>
#include <functional>
#include <algorithm>
#include <cctype>
#include <iostream>

class Collection {
public:
    void setItems(const std::vector<Item>& items) {
        m_allItems = items;
        applyFilter();
        notifyFilterChanged();
    }
    
    void setFilter(const std::string& filter) {
        if (m_filterQuery == filter) return;
      
        m_filterQuery = filter;
        applyFilter();
        notifyFilterChanged();
    }
    
    const std::string& getFilter() const { return m_filterQuery; }
    size_t totalCount() const { return m_allItems.size(); }
    size_t visibleCount() const { return m_visibleIndices.size(); }
    
    const Item& getItem(size_t index) const { return m_allItems[index]; }
    
    const std::vector<size_t>& getVisibleIndices() const { 
        return m_visibleIndices; 
    }
    
    void onFilterChange(std::function<void()> cb) {
      
        m_onFilterChange.push_back(cb);
    }
    
private:
    void applyFilter() {
        m_visibleIndices.clear();
        
        if (m_filterQuery.empty()) {
            for (size_t i = 0; i < m_allItems.size(); ++i) 
                m_visibleIndices.push_back(i);
         
            return;
        }
        
        std::string query = m_filterQuery;
        std::transform(query.begin(), query.end(), query.begin(), ::tolower);
        
        for (size_t i = 0; i < m_allItems.size(); ++i) {
            std::string name = m_allItems[i].displayName();
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);
            if (name.find(query) != std::string::npos) 
                m_visibleIndices.push_back(i);
        }
        
        
    }
    
    void notifyFilterChanged() {
        for (auto& cb : m_onFilterChange) {
            cb();
        }
    }
    
    std::vector<Item> m_allItems;
    std::vector<size_t> m_visibleIndices;
    std::string m_filterQuery;
    
    std::vector<std::function<void()>> m_onFilterChange;
};
