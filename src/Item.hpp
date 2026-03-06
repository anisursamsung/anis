#pragma once

#include <hyprtoolkit/system/Icons.hpp>
#include <hyprutils/memory/SharedPtr.hpp>
#include <string>
#include <vector>
#include <filesystem>
#include <variant>
#include <iostream>

namespace fs = std::filesystem;

// -----------------------------------------------------------------
// FULL STRUCT DEFINITIONS
// -----------------------------------------------------------------
struct AppItem {
    std::string displayName;
    std::string exec;
    std::string desktopPath;
    Hyprutils::Memory::CSharedPointer<Hyprtoolkit::ISystemIconDescription> iconDesc;
    std::string iconPath;
    
    std::string getIconSource() const {
        if (iconDesc) return "icon:" + displayName;
        return iconPath.empty() ? "application-x-executable" : iconPath;
    }
    
    void activate() const {
        if (exec.empty()) return;
        
        std::string cmd = exec;
        size_t pos = 0;
        while ((pos = cmd.find('%', pos)) != std::string::npos) {
            if (pos + 1 < cmd.length()) {
                char next = cmd[pos + 1];
                if (next == 'U' || next == 'F' || next == 'u' || next == 'f' || 
                    next == 'N' || next == 'n' || next == 'd' || next == 'D' || 
                    next == 'v' || next == 'm') {
                    cmd.erase(pos, 2);
                    continue;
                }
            }
            pos++;
        }
        while (!cmd.empty() && (cmd.back() == ' ' || cmd.back() == '\t')) {
            cmd.pop_back();
        }
        
        std::system((cmd + " &").c_str());
    }
};

struct FileItem {
    std::string displayName;
    fs::path filePath;
    fs::path thumbnailPath;
    uintmax_t fileSize = 0;
    fs::file_time_type modifiedTime;
    std::string onClickCommand;
    
    std::string getIconSource() const {
        if (!thumbnailPath.empty() && fs::exists(thumbnailPath))
            return thumbnailPath.string();
        if (fs::exists(filePath))
            return filePath.string();
        return "image-x-generic";
    }
    
    void activate() const {
        // Check for custom onClick command first
        if (!onClickCommand.empty()) {
            std::string cmd = onClickCommand;
            // Replace %f with file path
            size_t pos = 0;
            while ((pos = cmd.find("%f", pos)) != std::string::npos) {
                cmd.replace(pos, 2, filePath.string());
                pos += filePath.string().length();
            }
            // Replace %n with display name
            pos = 0;
            while ((pos = cmd.find("%n", pos)) != std::string::npos) {
                cmd.replace(pos, 2, displayName);
                pos += displayName.length();
            }
            std::system((cmd + " &").c_str());
        }
        // Default behavior
        else if (fs::exists(filePath)) {
        std::cout << filePath.string() << std::endl;
        }
    }
};

struct OptionItem {
    std::string displayName;
    std::string iconPath;
    Hyprutils::Memory::CSharedPointer<Hyprtoolkit::ISystemIconDescription> iconDesc;
    std::string action;
    bool isExecutable;
    std::string onClickCommand;
    
    std::string getIconSource() const {
        if (iconDesc) return "icon:" + displayName;
        if (!iconPath.empty() && fs::exists(iconPath))
            return iconPath;
        if (!iconPath.empty())
            return iconPath;
        return "text-x-generic";
    }
    
    void activate() const {
        // Check for custom onClick command first
        if (!onClickCommand.empty()) {
            std::string cmd = onClickCommand;
            // Replace %a with action
            size_t pos = 0;
            while ((pos = cmd.find("%a", pos)) != std::string::npos) {
                cmd.replace(pos, 2, action);
                pos += action.length();
            }
            // Replace %n with display name
            pos = 0;
            while ((pos = cmd.find("%n", pos)) != std::string::npos) {
                cmd.replace(pos, 2, displayName);
                pos += displayName.length();
            }
            std::system((cmd + " &").c_str());
        }
        // Default behavior
        else if (isExecutable && !action.empty()) {
            std::system((action + " &").c_str());
        } else {
          std::cout << action << std::endl;
        }
    }
};

// -----------------------------------------------------------------
// MAIN ITEM STRUCT
// -----------------------------------------------------------------
struct Item {
    std::variant<std::monostate, AppItem, FileItem, OptionItem> data;
    
    // Constructors
    Item() = default;
    Item(const AppItem& app) : data(app) {}
    Item(const FileItem& file) : data(file) {}
    Item(const OptionItem& opt) : data(opt) {}
    
    // Accessors
    std::string displayName() const {
        if (std::holds_alternative<AppItem>(data))
            return std::get<AppItem>(data).displayName;
        if (std::holds_alternative<FileItem>(data))
            return std::get<FileItem>(data).displayName;
        if (std::holds_alternative<OptionItem>(data))
            return std::get<OptionItem>(data).displayName;
        return "";
    }
    
    std::string iconSource() const {
        if (std::holds_alternative<AppItem>(data))
            return std::get<AppItem>(data).getIconSource();
        if (std::holds_alternative<FileItem>(data))
            return std::get<FileItem>(data).getIconSource();
        if (std::holds_alternative<OptionItem>(data))
            return std::get<OptionItem>(data).getIconSource();
        return "";
    }
    
    void activate() const {
        if (std::holds_alternative<AppItem>(data))
            std::get<AppItem>(data).activate();
        else if (std::holds_alternative<FileItem>(data))
            std::get<FileItem>(data).activate();
        else if (std::holds_alternative<OptionItem>(data))
            std::get<OptionItem>(data).activate();
    }
    
    bool isApp() const { return std::holds_alternative<AppItem>(data); }
    bool isFile() const { return std::holds_alternative<FileItem>(data); }
    bool isOption() const { return std::holds_alternative<OptionItem>(data); }
    bool isValid() const { return !std::holds_alternative<std::monostate>(data); }
};

// -----------------------------------------------------------------
// FACTORY FUNCTIONS
// -----------------------------------------------------------------
namespace ItemFactory {
    inline Item makeApp(
        const std::string& name,
        const std::string& exec,
        const std::string& desktopPath = "",
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::ISystemIconDescription> iconDesc = nullptr,
        const std::string& iconPath = ""
    ) {
        AppItem app;
        app.displayName = name;
        app.exec = exec;
        app.desktopPath = desktopPath;
        app.iconDesc = iconDesc;
        app.iconPath = iconPath;
        return Item(app);
    }
    
    inline Item makeFile(
        const fs::path& filePath,
        const fs::path& thumbnailPath = "",
        const std::string& onClick = ""
    ) {
        FileItem file;
        file.displayName = filePath.filename().string();
        file.filePath = filePath;
        file.thumbnailPath = thumbnailPath;
        file.onClickCommand = onClick;
        
        try {
            if (fs::exists(filePath)) {
                file.fileSize = fs::file_size(filePath);
                file.modifiedTime = fs::last_write_time(filePath);
            }
        } catch (...) {}
        
        return Item(file);
    }
    
    inline Item makeOption(
        const std::string& name,
        const std::string& action,
        bool isExecutable = false,
        const std::string& iconPath = "",
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::ISystemIconDescription> iconDesc = nullptr,
        const std::string& onClick = ""
    ) {
        OptionItem opt;
        opt.displayName = name;
        opt.action = action;
        opt.isExecutable = isExecutable;
        opt.iconPath = iconPath;
        opt.iconDesc = iconDesc;
        opt.onClickCommand = onClick;
        return Item(opt);
    }
}
