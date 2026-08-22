#pragma once

#include <hyprtoolkit/system/Icons.hpp>
#include <hyprutils/memory/SharedPtr.hpp>
#include <string>
#include <vector>
#include <filesystem>
#include <variant>
#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>

namespace fs = std::filesystem;

namespace HLMenu {

// ============================================================================
// ITEM DATA MODELS
// ============================================================================
// An "Item" represents any selectable entry displayed in hlmenu.
// hlmenu supports three core types of items:
//   1. AppItem    - Desktop applications parsed from .desktop files
//   2. FileItem   - Files or image thumbnails scanned from directories
//   3. OptionItem - Custom piped options (e.g. power menus, scripts, rofi-style)
// ============================================================================

/**
 * @struct AppItem
 * @brief Represents an installed desktop application.
 */
struct AppItem {
    std::string displayName;   ///< User-visible application name (e.g. "Firefox")
    std::string description;   ///< GenericName or Comment (e.g. "Web Browser")
    std::string exec;          ///< Executable launch command from the .desktop file
    std::string desktopPath;   ///< Full path to the .desktop entry
    Hyprutils::Memory::CSharedPointer<Hyprtoolkit::ISystemIconDescription> iconDesc; ///< System icon handle
    std::string iconPath;      ///< Absolute file path to icon if not in theme

    /**
     * @brief Returns a string identifier or file path for the item's icon.
     */
    std::string getIconSource() const {
        if (iconDesc) return "icon:" + displayName;
        return iconPath.empty() ? "application-x-executable" : iconPath;
    }

    /**
     * @brief Spawns the desktop application in the background.
     * Removes FreeDesktop field codes (e.g. %u, %F) from the exec string.
     */
    void activate() const {
        if (exec.empty()) return;

        std::string cmd = exec;
        size_t pos = 0;
        // Strip FreeDesktop field codes (%u, %f, %F, etc.)
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
        // Trim trailing whitespace
        while (!cmd.empty() && (cmd.back() == ' ' || cmd.back() == '\t')) {
            cmd.pop_back();
        }

        // Launch detached process
        std::system((cmd + " &").c_str());
    }
};

/**
 * @struct RunItem
 * @brief Represents an executable binary command found in $PATH.
 */
struct RunItem {
    std::string name;          ///< Binary executable name (e.g. "htop", "nvim")
    std::string path;          ///< Absolute filesystem path (e.g. "/usr/bin/htop")
    Hyprutils::Memory::CSharedPointer<Hyprtoolkit::ISystemIconDescription> iconDesc; ///< Resolved system icon handle
    std::string iconPath;      ///< Absolute icon file path if resolved

    /**
     * @brief Returns an icon source identifier for the binary.
     */
    std::string getIconSource() const {
        if (iconDesc) return "icon:" + name;
        if (!iconPath.empty() && fs::exists(iconPath)) return iconPath;
        if (!iconPath.empty()) return iconPath;
        return "system-run";
    }

    /**
     * @brief Launches the executable binary in the background.
     */
    void activate() const {
        if (path.empty()) return;
        std::string cmd = path + " &";
        std::system(cmd.c_str());
    }
};

/**
 * @struct FileItem
 * @brief Represents a file or image entry in file browser mode.
 */
struct FileItem {
    std::string displayName;       ///< File name or title
    fs::path filePath;             ///< Absolute path to the file
    std::string iconName;          ///< Optional icon name/placeholder (e.g. "text-x-generic", "folder")
    fs::path thumbnailPath;        ///< Cached thumbnail image path (if generated)
    uintmax_t fileSize = 0;        ///< File size in bytes
    fs::file_time_type modifiedTime; ///< Last modification timestamp
    std::string onClickCommand;    ///< Custom command template (%f=path, %n=name)

    /**
     * @brief Returns the best icon or thumbnail path for the file.
     */
    std::string getIconSource() const {
        if (!iconName.empty())
            return iconName;
        if (!thumbnailPath.empty() && fs::exists(thumbnailPath))
            return thumbnailPath.string();
        if (fs::exists(filePath))
            return filePath.string();
        return "text-x-generic";
    }

    /**
     * @brief Executes the file or runs a custom onClick command.
     */
    void activate() const {
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
        } else if (fs::exists(filePath)) {
            // Default: print path to stdout (for script piping)
            std::cout << filePath.string() << std::endl;
        }
    }
};

/**
 * @struct OptionItem
 * @brief Represents a custom menu option (e.g. rofi/dmenu style).
 */
struct OptionItem {
    std::string displayName;       ///< Label displayed to user
    std::string iconPath;          ///< Icon name or path
    Hyprutils::Memory::CSharedPointer<Hyprtoolkit::ISystemIconDescription> iconDesc; ///< System icon handle
    std::string action;            ///< Action string, shell command, or return value
    bool isExecutable = false;     ///< True if action should be executed in shell
    std::string onClickCommand;    ///< Custom command template (%a=action, %n=name)

    /**
     * @brief Returns the icon identifier.
     */
    std::string getIconSource() const {
        if (iconDesc) return "icon:" + displayName;
        if (!iconPath.empty() && fs::exists(iconPath)) return iconPath;
        if (!iconPath.empty()) return iconPath;
        return "text-x-generic";
    }

    /**
     * @brief Executes the action or outputs it to standard output.
     */
    void activate() const {
        if (!onClickCommand.empty()) {
            std::string cmd = onClickCommand;
            size_t pos = 0;
            while ((pos = cmd.find("%a", pos)) != std::string::npos) {
                cmd.replace(pos, 2, action);
                pos += action.length();
            }
            pos = 0;
            while ((pos = cmd.find("%n", pos)) != std::string::npos) {
                cmd.replace(pos, 2, displayName);
                pos += displayName.length();
            }
            std::system((cmd + " &").c_str());
        } else if (isExecutable && !action.empty()) {
            std::system((action + " &").c_str());
        } else {
            std::cout << action << std::endl;
        }
    }
};

/**
 * @struct WindowItem
 * @brief Represents an active Hyprland window/client.
 */
struct WindowItem {
    std::string title;             ///< Window title (e.g. "Google Chrome")
    std::string windowClass;       ///< Window application class (e.g. "google-chrome")
    std::string address;           ///< Hyprland window address (e.g. "0x55ae37989950")
    std::string workspaceName;     ///< Workspace name/ID (e.g. "1")
    Hyprutils::Memory::CSharedPointer<Hyprtoolkit::ISystemIconDescription> iconDesc; ///< Resolved system icon
    std::string iconPath;          ///< Icon file path if resolved

    std::string getIconSource() const {
        if (iconDesc) return "icon:" + title;
        if (!iconPath.empty()) return iconPath;
        if (!windowClass.empty()) return windowClass;
        return "application-x-executable";
    }

    std::string getSubtitle() const {
        std::string sub = "[" + workspaceName + "]";
        if (!windowClass.empty()) {
            sub += " " + windowClass;
        }
        return sub;
    }

    void activate() const {
        if (address.empty()) return;

        // 1. Direct Hyprland IPC socket dispatch using Hyprland Lua API
        const char* xdg = std::getenv("XDG_RUNTIME_DIR");
        const char* his = std::getenv("HYPRLAND_INSTANCE_SIGNATURE");
        if (his) {
            std::string sockPath;
            if (xdg) {
                sockPath = std::string(xdg) + "/hypr/" + his + "/.socket.sock";
            }
            if (sockPath.empty() || !std::filesystem::exists(sockPath)) {
                sockPath = "/tmp/hypr/" + std::string(his) + "/.socket.sock";
            }

            int sock = socket(AF_UNIX, SOCK_STREAM, 0);
            if (sock >= 0) {
                struct sockaddr_un addr;
                std::memset(&addr, 0, sizeof(addr));
                addr.sun_family = AF_UNIX;
                std::strncpy(addr.sun_path, sockPath.c_str(), sizeof(addr.sun_path) - 1);

                if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
                    std::string req = "eval hl.dispatch(hl.dsp.focus({ window = \"address:" + address + "\" }))";
                    send(sock, req.c_str(), req.length(), 0);
                    close(sock);
                    return;
                }
                close(sock);
            }
        }

        // 2. Fallback to hyprctl eval command
        std::string cmd = "hyprctl eval 'hl.dispatch(hl.dsp.focus({ window = \"address:" + address + "\" }))' &";
        std::system(cmd.c_str());
    }
};

/**
 * @struct WorkspaceItem
 * @brief Represents an active Hyprland workspace.
 */
struct WorkspaceItem {
    int id = 1;
    std::string name;              ///< Workspace name (e.g. "1", "3", "special:scratchpad")
    std::string monitor;           ///< Monitor name (e.g. "eDP-1")
    int windowCount = 0;           ///< Number of open windows in this workspace
    std::string lastWindowTitle;   ///< Title of active window on this workspace
    bool isActive = false;         ///< True if this is the currently focused workspace
    Hyprutils::Memory::CSharedPointer<Hyprtoolkit::ISystemIconDescription> iconDesc;

    std::string getDisplayName() const {
        return "Workspace " + name;
    }

    std::string getSubtitle() const {
        std::string sub;
        if (!lastWindowTitle.empty()) {
            sub += lastWindowTitle + " • ";
        }
        sub += std::to_string(windowCount) + (windowCount == 1 ? " window" : " windows");
        if (!monitor.empty()) {
            sub += " (" + monitor + ")";
        }
        if (isActive) {
            sub += " [Active]";
        }
        return sub;
    }

    std::string getIconSource() const {
        if (iconDesc) return "icon:" + name;
        return "preferences-desktop-display";
    }

    void activate() const {
        if (name.empty()) return;

        // 1. Direct Hyprland IPC socket dispatch
        const char* xdg = std::getenv("XDG_RUNTIME_DIR");
        const char* his = std::getenv("HYPRLAND_INSTANCE_SIGNATURE");
        if (his) {
            std::string sockPath;
            if (xdg) {
                sockPath = std::string(xdg) + "/hypr/" + his + "/.socket.sock";
            }
            if (sockPath.empty() || !std::filesystem::exists(sockPath)) {
                sockPath = "/tmp/hypr/" + std::string(his) + "/.socket.sock";
            }

            int sock = socket(AF_UNIX, SOCK_STREAM, 0);
            if (sock >= 0) {
                struct sockaddr_un addr;
                std::memset(&addr, 0, sizeof(addr));
                addr.sun_family = AF_UNIX;
                std::strncpy(addr.sun_path, sockPath.c_str(), sizeof(addr.sun_path) - 1);

                if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
                    std::string req = "eval hl.dispatch(hl.dsp.focus({ workspace = \"" + name + "\" }))";
                    send(sock, req.c_str(), req.length(), 0);
                    close(sock);
                    return;
                }
                close(sock);
            }
        }

        // 2. Fallback to hyprctl command
        std::string cmd = "hyprctl eval 'hl.dispatch(hl.dsp.focus({ workspace = \"" + name + "\" }))' &";
        std::system(cmd.c_str());
    }
};

// ============================================================================
// UNIVERSAL ITEM CONTAINER
// ============================================================================
/**
 * @struct Item
 * @brief Universal polymorphic container using std::variant to hold
 *        an AppItem, FileItem, OptionItem, WindowItem, or WorkspaceItem with zero heap overhead.
 */
struct Item {
    std::variant<std::monostate, AppItem, RunItem, FileItem, OptionItem, WindowItem, WorkspaceItem> data;

    Item() = default;
    Item(const AppItem& app) : data(app) {}
    Item(const RunItem& run) : data(run) {}
    Item(const FileItem& file) : data(file) {}
    Item(const OptionItem& opt) : data(opt) {}
    Item(const WindowItem& win) : data(win) {}
    Item(const WorkspaceItem& ws) : data(ws) {}

    /// @brief Retrieve the display name regardless of underlying item type
    std::string displayName() const {
        if (std::holds_alternative<AppItem>(data))
            return std::get<AppItem>(data).displayName;
        if (std::holds_alternative<RunItem>(data))
            return std::get<RunItem>(data).name;
        if (std::holds_alternative<FileItem>(data))
            return std::get<FileItem>(data).displayName;
        if (std::holds_alternative<OptionItem>(data))
            return std::get<OptionItem>(data).displayName;
        if (std::holds_alternative<WindowItem>(data))
            return std::get<WindowItem>(data).title;
        if (std::holds_alternative<WorkspaceItem>(data))
            return std::get<WorkspaceItem>(data).getDisplayName();
        return "";
    }

    /// @brief Retrieve subtitle/description text for list rows and grid cards
    std::string subtitle() const {
        if (std::holds_alternative<AppItem>(data))
            return std::get<AppItem>(data).description;
        if (std::holds_alternative<RunItem>(data))
            return std::get<RunItem>(data).path;
        if (std::holds_alternative<FileItem>(data)) {
            const auto& f = std::get<FileItem>(data);
            return f.filePath.parent_path().string();
        }
        if (std::holds_alternative<OptionItem>(data)) {
            const auto& opt = std::get<OptionItem>(data);
            if (opt.action.empty() || opt.action == opt.displayName) {
                return "";
            }
            return opt.action;
        }
        if (std::holds_alternative<WindowItem>(data))
            return std::get<WindowItem>(data).getSubtitle();
        if (std::holds_alternative<WorkspaceItem>(data))
            return std::get<WorkspaceItem>(data).getSubtitle();
        return "";
    }

    /// @brief Retrieve the icon source identifier
    std::string iconSource() const {
        if (std::holds_alternative<AppItem>(data))
            return std::get<AppItem>(data).getIconSource();
        if (std::holds_alternative<RunItem>(data))
            return std::get<RunItem>(data).getIconSource();
        if (std::holds_alternative<FileItem>(data))
            return std::get<FileItem>(data).getIconSource();
        if (std::holds_alternative<OptionItem>(data))
            return std::get<OptionItem>(data).getIconSource();
        if (std::holds_alternative<WindowItem>(data))
            return std::get<WindowItem>(data).getIconSource();
        if (std::holds_alternative<WorkspaceItem>(data))
            return std::get<WorkspaceItem>(data).getIconSource();
        return "";
    }

    /// @brief Trigger activation (launch application, run binary, focus window, switch workspace, run action)
    void activate() const {
        if (std::holds_alternative<AppItem>(data))
            std::get<AppItem>(data).activate();
        else if (std::holds_alternative<RunItem>(data))
            std::get<RunItem>(data).activate();
        else if (std::holds_alternative<FileItem>(data))
            std::get<FileItem>(data).activate();
        else if (std::holds_alternative<OptionItem>(data))
            std::get<OptionItem>(data).activate();
        else if (std::holds_alternative<WindowItem>(data))
            std::get<WindowItem>(data).activate();
        else if (std::holds_alternative<WorkspaceItem>(data))
            std::get<WorkspaceItem>(data).activate();
    }

    bool isApp() const { return std::holds_alternative<AppItem>(data); }
    bool isRun() const { return std::holds_alternative<RunItem>(data); }
    bool isFile() const { return std::holds_alternative<FileItem>(data); }
    bool isOption() const { return std::holds_alternative<OptionItem>(data); }
    bool isWindow() const { return std::holds_alternative<WindowItem>(data); }
    bool isWorkspace() const { return std::holds_alternative<WorkspaceItem>(data); }
    bool isValid() const { return !std::holds_alternative<std::monostate>(data); }
};

// ============================================================================
// CONVENIENCE FACTORIES
// ============================================================================
namespace ItemFactory {

    inline Item makeApp(
        const std::string& name,
        const std::string& exec,
        const std::string& desktopPath = "",
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::ISystemIconDescription> iconDesc = nullptr,
        const std::string& iconPath = "",
        const std::string& description = ""
    ) {
        AppItem app;
        app.displayName = name;
        app.exec = exec;
        app.desktopPath = desktopPath;
        app.iconDesc = iconDesc;
        app.iconPath = iconPath;
        app.description = description;
        return Item(app);
    }

    inline Item makeRun(
        const std::string& name,
        const std::string& path,
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::ISystemIconDescription> iconDesc = nullptr,
        const std::string& iconPath = ""
    ) {
        RunItem run;
        run.name = name;
        run.path = path;
        run.iconDesc = iconDesc;
        run.iconPath = iconPath;
        return Item(run);
    }

    inline Item makeImage(
        const fs::path& filePath,
        const fs::path& thumbnailPath = "",
        const std::string& onClick = ""
    ) {
        FileItem file;
        file.displayName = filePath.filename().string();
        file.filePath = filePath;
        file.iconName = ""; // Empty so filePath is used as direct image icon
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

    inline Item makeFile(
        const fs::path& filePath,
        bool isDirectory = false,
        const std::string& onClick = "",
        const std::string& customIcon = ""
    ) {
        FileItem file;
        file.displayName = filePath.filename().string();
        file.filePath = filePath;
        if (!customIcon.empty()) {
            file.iconName = customIcon;
        } else if (isDirectory) {
            file.iconName = "folder";
        } else {
            file.iconName = "text-x-generic";
        }
        file.onClickCommand = onClick;

        try {
            if (fs::exists(filePath) && !isDirectory) {
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

    inline Item makeWindow(
        const std::string& title,
        const std::string& windowClass,
        const std::string& address,
        const std::string& workspaceName,
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::ISystemIconDescription> iconDesc = nullptr,
        const std::string& iconPath = ""
    ) {
        WindowItem win;
        win.title = title;
        win.windowClass = windowClass;
        win.address = address;
        win.workspaceName = workspaceName;
        win.iconDesc = iconDesc;
        win.iconPath = iconPath;
        return Item(win);
    }

    inline Item makeWorkspace(
        int id,
        const std::string& name,
        const std::string& monitor = "",
        int windowCount = 0,
        const std::string& lastWindowTitle = "",
        bool isActive = false,
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::ISystemIconDescription> iconDesc = nullptr
    ) {
        WorkspaceItem ws;
        ws.id = id;
        ws.name = name;
        ws.monitor = monitor;
        ws.windowCount = windowCount;
        ws.lastWindowTitle = lastWindowTitle;
        ws.isActive = isActive;
        ws.iconDesc = iconDesc;
        return Item(ws);
    }

} // namespace ItemFactory

} // namespace HLMenu
