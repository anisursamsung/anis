#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include "Item.hpp"

class OptionParser {
public:
    // Parse "Save,Load,Quit" -> vector of OptionItems (echo)
    static std::vector<Item> parseSimpleList(const std::string& source, const std::string& onClick = "") {
        std::vector<Item> items;
        std::stringstream ss(source);
        std::string item;
        
        while (std::getline(ss, item, ',')) {
            // Trim whitespace
            item.erase(0, item.find_first_not_of(" \t"));
            item.erase(item.find_last_not_of(" \t") + 1);
            
            if (!item.empty()) {
                items.push_back(ItemFactory::makeOption(item, item, false, "", nullptr, onClick));
            }
        }
        
        return items;
    }
    
    // Parse "firefox.png,Firefox;chrome.png,Chrome" -> vector of OptionItems (echo with icons)
    // Or "firefox.png,Firefox,/usr/bin/firefox;code.png,VS Code,/usr/bin/code" -> (exec with icons)
    static std::vector<Item> parseStructuredList(const std::string& source, const std::string& onClick = "") {
        std::vector<Item> items;
        std::stringstream ss(source);
        std::string entry;
        
        while (std::getline(ss, entry, ';')) {
            if (entry.empty()) continue;
            
            std::vector<std::string> fields;
            std::stringstream entryStream(entry);
            std::string field;
            
            while (std::getline(entryStream, field, ',')) {
                // Trim whitespace
                field.erase(0, field.find_first_not_of(" \t"));
                field.erase(field.find_last_not_of(" \t") + 1);
                fields.push_back(field);
            }
            
            if (fields.size() == 1) {
                // Just name
                items.push_back(ItemFactory::makeOption(fields[0], fields[0], false, "", nullptr, onClick));
            }
            else if (fields.size() == 2) {
                // Icon, Name (echo)
                items.push_back(ItemFactory::makeOption(fields[1], fields[1], false, fields[0], nullptr, onClick));
            }
            else if (fields.size() >= 3) {
                // Icon, Name, Command (exec)
                // Reconstruct name if it had commas (like "VS Code")
                std::string name = fields[1];
                for (size_t i = 2; i < fields.size() - 1; i++) {
                    name += "," + fields[i];
                }
                std::string command = fields.back();
                
                items.push_back(ItemFactory::makeOption(name, command, true, fields[0], nullptr, onClick));
            }
        }
        
        return items;
    }
    
    // Detect format and parse accordingly
    static std::vector<Item> parseOptions(const std::string& source, const std::string& onClick = "") {
        // Check if it's structured format (contains ';')
        if (source.find(';') != std::string::npos) {
            return parseStructuredList(source, onClick);
        }
        // Default to simple comma-separated list
        return parseSimpleList(source, onClick);
    }
};
