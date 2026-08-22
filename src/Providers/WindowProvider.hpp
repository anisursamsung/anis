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
// WINDOW PROVIDER - HYPRLAND OPEN WINDOW / CLIENT SCANNER
// ============================================================================
// WindowProvider queries Hyprland's IPC socket to enumerate all active open
// windows, resolves their application icons, and creates WindowItems that focus
// the window upon selection.
// ============================================================================

class WindowProvider {
public:
    /**
     * @brief Queries Hyprland for all mapped client windows.
     * @param backend Shared pointer to Hyprtoolkit backend for icon lookup.
     * @return std::vector<Item> List of open window items.
     */
    static std::vector<Item> getWindows(Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IBackend> backend) {
        std::vector<Item> items;
        std::string jsonStr = queryClientsJson();

        if (jsonStr.empty()) {
            return items;
        }

        auto iconFactory = backend ? backend->systemIcons() : nullptr;

        // Parse JSON client objects manually for speed & zero extra dependencies
        size_t pos = 0;
        while ((pos = jsonStr.find('{', pos)) != std::string::npos) {
            size_t endObj = findMatchingBrace(jsonStr, pos);
            if (endObj == std::string::npos) break;

            std::string objStr = jsonStr.substr(pos, endObj - pos + 1);
            pos = endObj + 1;

            // Extract fields
            std::string address = extractJsonString(objStr, "address");
            std::string winClass = extractJsonString(objStr, "class");
            std::string initialClass = extractJsonString(objStr, "initialClass");
            std::string title = extractJsonString(objStr, "title");
            std::string wsName = extractWorkspaceName(objStr);
            bool mapped = extractJsonBool(objStr, "mapped", true);
            bool hidden = extractJsonBool(objStr, "hidden", false);

            if (!mapped || hidden || address.empty()) {
                continue;
            }

            if (title.empty()) {
                title = !winClass.empty() ? winClass : "Window (" + address + ")";
            }

            // Lookup window icon
            Hyprutils::Memory::CSharedPointer<Hyprtoolkit::ISystemIconDescription> iconDesc = nullptr;
            if (iconFactory) {
                if (!winClass.empty()) {
                    iconDesc = iconFactory->lookupIcon(winClass);
                    if (!iconDesc || !iconDesc->exists()) {
                        std::string lowerClass = winClass;
                        std::transform(lowerClass.begin(), lowerClass.end(), lowerClass.begin(), ::tolower);
                        iconDesc = iconFactory->lookupIcon(lowerClass);
                    }
                }
                if ((!iconDesc || !iconDesc->exists()) && !initialClass.empty()) {
                    std::string lowerInit = initialClass;
                    std::transform(lowerInit.begin(), lowerInit.end(), lowerInit.begin(), ::tolower);
                    iconDesc = iconFactory->lookupIcon(lowerInit);
                }
                if (!iconDesc || !iconDesc->exists()) {
                    iconDesc = iconFactory->lookupIcon("application-x-executable");
                }
            }

            items.push_back(ItemFactory::makeWindow(title, winClass, address, wsName, iconDesc, ""));
        }

        return items;
    }

private:
    /**
     * @brief Queries Hyprland IPC socket (or falls back to hyprctl).
     */
    static std::string queryClientsJson() {
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
                    const char* req = "j/clients";
                    send(sock, req, std::strlen(req), 0);

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

        // Fallback: popen hyprctl -j clients
        FILE* fp = popen("hyprctl -j clients 2>/dev/null", "r");
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
        // Skip whitespace
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

    static bool extractJsonBool(const std::string& obj, const std::string& key, bool defaultVal) {
        std::string pattern = "\"" + key + "\":";
        size_t pos = obj.find(pattern);
        if (pos == std::string::npos) return defaultVal;

        pos += pattern.length();
        while (pos < obj.length() && (obj[pos] == ' ' || obj[pos] == '\t')) pos++;

        if (obj.compare(pos, 4, "true") == 0) return true;
        if (obj.compare(pos, 5, "false") == 0) return false;
        return defaultVal;
    }

    static std::string extractWorkspaceName(const std::string& obj) {
        size_t wsPos = obj.find("\"workspace\":");
        if (wsPos == std::string::npos) return "1";

        size_t namePos = obj.find("\"name\":", wsPos);
        if (namePos != std::string::npos) {
            namePos += 7;
            while (namePos < obj.length() && (obj[namePos] == ' ' || obj[namePos] == '"')) namePos++;
            size_t end = obj.find('"', namePos);
            if (end != std::string::npos) {
                return obj.substr(namePos, end - namePos);
            }
        }

        size_t idPos = obj.find("\"id\":", wsPos);
        if (idPos != std::string::npos) {
            idPos += 5;
            while (idPos < obj.length() && (obj[idPos] == ' ' || obj[idPos] == '\t')) idPos++;
            size_t end = obj.find_first_of(",}\n\r", idPos);
            if (end != std::string::npos) {
                return obj.substr(idPos, end - idPos);
            }
        }

        return "1";
    }
};

} // namespace HLMenu
