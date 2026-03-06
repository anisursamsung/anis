#pragma once

#include <hyprtoolkit/system/Icons.hpp>
#include <hyprutils/memory/SharedPtr.hpp>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

struct IconResult {
    enum class Type {
        NONE,
        ICON_DESCRIPTION,
        PATH
    };
    
    Type type = Type::NONE;
    
    // Common - EVERY IconResult has these!
    std::string displayName;  // What to show in UI ("Firefox", "vacation.jpg")
    std::string exec;         // What to execute: exec command for apps, file path for images
    std::string iconPath;     // Icon name or path for finding the icon
    
    // For ICON_DESCRIPTION (system theme icons)
    Hyprutils::Memory::CSharedPointer<Hyprtoolkit::ISystemIconDescription> iconDesc;
    
    // For PATH (filesystem paths)
    std::string path;         // Path to image file (used as icon)
    
    // -----------------------------------------------------------------
    // FACTORY METHODS
    // -----------------------------------------------------------------
    
    static IconResult makeIconDesc(const std::string& name,
                                   const std::string& icon,
                                   const std::string& execCmd,
                                   Hyprutils::Memory::CSharedPointer<Hyprtoolkit::ISystemIconDescription> desc) {
        IconResult result;
        result.type = Type::ICON_DESCRIPTION;
        result.displayName = name;
        result.exec = execCmd;          // ← Stores exec command
        result.iconPath = icon;          // ← Stores icon name
        result.iconDesc = desc;
        return result;
    }
    
    static IconResult makePath(const std::string& name,
                               const std::string& icon,
                               const std::string& execCmd,
                               const std::string& path) {
        IconResult result;
        result.type = Type::PATH;
        result.displayName = name;
        result.exec = execCmd;           // ← Stores exec command or file path
        result.iconPath = icon;           // ← Stores icon name/path
        result.path = path;               // ← Stores path to image file
        return result;
    }
    
    static IconResult makeNone(const std::string& name = "", const std::string& execCmd = "") {
        IconResult result;
        result.type = Type::NONE;
        result.displayName = name;
        result.exec = execCmd;
        return result;
    }
    
    // -----------------------------------------------------------------
    // HELPERS
    // -----------------------------------------------------------------
    
    bool isValid() const { return type != Type::NONE; }
    bool isIconDesc() const { return type == Type::ICON_DESCRIPTION; }
    bool isPath() const { return type == Type::PATH; }
};
