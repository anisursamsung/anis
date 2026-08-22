#pragma once

#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/system/Icons.hpp>
#include <hyprutils/memory/SharedPtr.hpp>
#include <vector>
#include <string>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include "Core/Item.hpp"

namespace HLMenu {

// ============================================================================
// WORKSPACE PROVIDER - HYPRLAND ACTIVE WORKSPACE SCANNER
// ============================================================================
// WorkspaceProvider queries Hyprland's IPC socket to enumerate all active
// workspaces, formats descriptions with window counts and active titles,
// and produces WorkspaceItems that switch workspaces upon selection.
// ============================================================================

class WorkspaceProvider {
public:
    /**
     * @brief Queries Hyprland for all active workspaces.
     * @param backend Shared pointer to Hyprtoolkit backend for icon lookup.
     * @return std::vector<Item> List of workspace items sorted by workspace ID.
     */
    static std::vector<Item> getWorkspaces(Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IBackend> backend) {
        std::vector<Item> items;
        std::string jsonStr = querySocketOrCommand("j/workspaces", "hyprctl -j workspaces 2>/dev/null");

        if (jsonStr.empty()) {
            return items;
        }

        // Get active workspace ID
        std::string activeJson = querySocketOrCommand("j/activeworkspace", "hyprctl -j activeworkspace 2>/dev/null");
        int activeId = -999;
        if (!activeJson.empty()) {
            activeId = extractJsonInt(activeJson, "id", -999);
        }

        auto iconFactory = backend ? backend->systemIcons() : nullptr;
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::ISystemIconDescription> wsIcon = nullptr;
        if (iconFactory) {
            wsIcon = iconFactory->lookupIcon("preferences-desktop-display");
            if (!wsIcon || !wsIcon->exists()) {
                wsIcon = iconFactory->lookupIcon("video-display");
            }
        }

        struct WsData {
            int id = 0;
            std::string name;
            std::string monitor;
            int windows = 0;
            std::string lastTitle;
            bool isActive = false;
        };

        std::vector<WsData> wsList;

        // Parse JSON workspace array
        size_t pos = 0;
        while ((pos = jsonStr.find('{', pos)) != std::string::npos) {
            size_t endObj = findMatchingBrace(jsonStr, pos);
            if (endObj == std::string::npos) break;

            std::string objStr = jsonStr.substr(pos, endObj - pos + 1);
            pos = endObj + 1;

            WsData ws;
            ws.id = extractJsonInt(objStr, "id", 0);
            ws.name = extractJsonString(objStr, "name");
            if (ws.name.empty()) {
                ws.name = std::to_string(ws.id);
            }
            ws.monitor = extractJsonString(objStr, "monitor");
            ws.windows = extractJsonInt(objStr, "windows", 0);
            ws.lastTitle = extractJsonString(objStr, "lastwindowtitle");
            ws.isActive = (ws.id == activeId);

            wsList.push_back(std::move(ws));
        }

        // Sort workspaces by numeric ID
        std::sort(wsList.begin(), wsList.end(), [](const WsData& a, const WsData& b) {
            return a.id < b.id;
        });

        for (const auto& ws : wsList) {
            items.push_back(ItemFactory::makeWorkspace(
                ws.id,
                ws.name,
                ws.monitor,
                ws.windows,
                ws.lastTitle,
                ws.isActive,
                wsIcon
            ));
        }

        return items;
    }

private:
    /**
     * @brief Queries Hyprland IPC socket or executes shell command.
     */
    static std::string querySocketOrCommand(const char* socketReq, const char* fallbackCmd) {
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
                    send(sock, socketReq, std::strlen(socketReq), 0);

                    std::string response;
                    char buf[8192];
                    ssize_t n;
                    while ((n = recv(sock, buf, sizeof(buf), 0)) > 0) {
                        response.append(buf, n);
                    }
                    close(sock);
                    if (!response.empty()) {
                        return response;
                    }
                } else {
                    close(sock);
                }
            }
        }

        // Fallback: popen command
        FILE* fp = popen(fallbackCmd, "r");
        if (!fp) return "";

        std::string result;
        char buffer[4096];
        while (fgets(buffer, sizeof(buffer), fp) != nullptr) {
            result += buffer;
        }
        pclose(fp);
        return result;
    }

    static size_t findMatchingBrace(const std::string& str, size_t start) {
        int depth = 0;
        for (size_t i = start; i < str.length(); ++i) {
            if (str[i] == '{') depth++;
            else if (str[i] == '}') {
                depth--;
                if (depth == 0) return i;
            }
        }
        return std::string::npos;
    }

    static std::string extractJsonString(const std::string& obj, const std::string& key) {
        std::string pattern = "\"" + key + "\":";
        size_t pos = obj.find(pattern);
        if (pos == std::string::npos) return "";

        pos += pattern.length();
        while (pos < obj.length() && (obj[pos] == ' ' || obj[pos] == '\t' || obj[pos] == '\n' || obj[pos] == '\r')) {
            pos++;
        }

        if (pos < obj.length() && obj[pos] == '"') {
            pos++;
            size_t end = pos;
            while (end < obj.length()) {
                if (obj[end] == '"' && obj[end - 1] != '\\') {
                    break;
                }
                end++;
            }
            return obj.substr(pos, end - pos);
        }
        return "";
    }

    static int extractJsonInt(const std::string& obj, const std::string& key, int defaultVal) {
        std::string pattern = "\"" + key + "\":";
        size_t pos = obj.find(pattern);
        if (pos == std::string::npos) return defaultVal;

        pos += pattern.length();
        while (pos < obj.length() && (obj[pos] == ' ' || obj[pos] == '\t')) pos++;

        size_t end = obj.find_first_of(",}\n\r", pos);
        if (end != std::string::npos) {
            std::string numStr = obj.substr(pos, end - pos);
            try {
                return std::stoi(numStr);
            } catch (...) {
                return defaultVal;
            }
        }
        return defaultVal;
    }
};

} // namespace HLMenu
