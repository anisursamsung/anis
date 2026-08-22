#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include "Core/Item.hpp"

namespace HLMenu {

// ============================================================================
// CUSTOM OPTIONS PARSER - DELIMITED & PIPED MENU INPUT
// ============================================================================
// CustomOptionsParser enables dmenu/rofi-style scripted menus by parsing
// simple or structured string inputs into interactive OptionItems:
//
// 1. Simple format:
//    "Shutdown, Reboot, Suspend, Lock"
//
// 2. Structured format with icons & shell commands:
//    "system-shutdown, Shutdown, systemctl poweroff; system-reboot, Reboot, systemctl reboot"
// ============================================================================

class CustomOptionsParser {
public:
    /**
     * @brief Parses a simple comma-separated string list.
     * Example: "Option 1, Option 2, Option 3"
     * @param source Delimited text input.
     * @param onClick Optional command template (%a=action, %n=name).
     */
    static std::vector<Item> parseSimpleList(const std::string& source, const std::string& onClick = "") {
        std::vector<Item> items;
        std::stringstream ss(source);
        std::string item;

        while (std::getline(ss, item, ',')) {
            // Trim leading and trailing whitespace
            item.erase(0, item.find_first_not_of(" \t"));
            item.erase(item.find_last_not_of(" \t") + 1);

            if (!item.empty()) {
                items.push_back(ItemFactory::makeOption(item, item, false, "", nullptr, onClick));
            }
        }

        return items;
    }

    /**
     * @brief Parses a structured list with icons and commands separated by semicolons.
     * Example: "firefox.png, Firefox, firefox; code.png, VS Code, code"
     * Format per entry: [icon, name] or [icon, name, command]
     */
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
                field.erase(0, field.find_first_not_of(" \t"));
                field.erase(field.find_last_not_of(" \t") + 1);
                fields.push_back(field);
            }

            if (fields.size() == 1) {
                // Name only (echo action)
                items.push_back(ItemFactory::makeOption(fields[0], fields[0], false, "", nullptr, onClick));
            }
            else if (fields.size() == 2) {
                // Icon + Name (echo action)
                items.push_back(ItemFactory::makeOption(fields[1], fields[1], false, fields[0], nullptr, onClick));
            }
            else if (fields.size() >= 3) {
                // Icon + Name + Executable command
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

    /**
     * @brief Parses newline-delimited lines (ideal for pipes and dmenu/rofi scripts).
     */
    static std::vector<Item> parseLines(const std::string& source, const std::string& onClick = "") {
        std::vector<Item> items;
        std::stringstream ss(source);
        std::string line;

        while (std::getline(ss, line)) {
            while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) {
                line.pop_back();
            }
            line.erase(0, line.find_first_not_of(" \t"));

            if (line.empty()) continue;

            // In newline-delimited mode, preserve the exact line string (including commas and symbols)
            items.push_back(ItemFactory::makeOption(line, line, false, "", nullptr, onClick));
        }

        return items;
    }

    /**
     * @brief Converts escaped literal '\n' sequences into actual newlines.
     */
    static std::string unescapeNewlines(const std::string& input) {
        std::string result;
        result.reserve(input.size());
        for (size_t i = 0; i < input.size(); ++i) {
            if (input[i] == '\\' && i + 1 < input.size() && input[i + 1] == 'n') {
                result.push_back('\n');
                ++i;
            } else {
                result.push_back(input[i]);
            }
        }
        return result;
    }

    /**
     * @brief Automatically detects whether input is simple, structured, or multiline and parses it.
     */
    static std::vector<Item> parseOptions(const std::string& rawSource, const std::string& onClick = "") {
        std::string source = unescapeNewlines(rawSource);

        if (source.find('\n') != std::string::npos) {
            return parseLines(source, onClick);
        }
        if (source.find(';') != std::string::npos) {
            return parseStructuredList(source, onClick);
        }
        return parseSimpleList(source, onClick);
    }
};

} // namespace HLMenu
