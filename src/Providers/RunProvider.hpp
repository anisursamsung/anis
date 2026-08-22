#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <unordered_set>
#include <algorithm>
#include <cstdlib>
#include <sstream>
#include "Core/Item.hpp"

namespace HLMenu {

namespace fs = std::filesystem;

// ============================================================================
// RUN PROVIDER - HIGH-PERFORMANCE SYSTEM EXECUTABLE DISCOVERY ($PATH SCANNER)
// ============================================================================
// RunProvider discovers all executable binaries available across standard
// system directories listed in the $PATH environment variable.
//
// Performance Optimizations:
//   1. In-memory static caching: Scans $PATH once per process session.
//   2. Zero-cost lazy icon binding: Avoids 5,000 synchronous disk theme lookups.
//   3. High-throughput directory iteration with duplicate suppression.
// ============================================================================

class RunProvider {
public:
    /**
     * @brief Scans $PATH directories and returns all unique executable binary items.
     * @param backend Shared pointer to Hyprtoolkit backend.
     * @param forceRefresh Whether to bypass the static cache and re-scan $PATH.
     * @return std::vector<Item> Sorted list of executable items.
     */
    static std::vector<Item> getExecutables(
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IBackend> backend = nullptr,
        bool forceRefresh = false
    ) {
        static std::vector<Item> s_cachedExecutables;
        static bool s_hasCached = false;

        if (s_hasCached && !forceRefresh) {
            return s_cachedExecutables;
        }

        std::vector<Item> items;
        items.reserve(4096);
        std::unordered_set<std::string> seen;

        const char* pathEnv = std::getenv("PATH");
        std::string pathStr = pathEnv ? pathEnv : "/usr/local/bin:/usr/bin:/bin:/usr/local/sbin:/usr/sbin";

        std::vector<fs::path> searchDirs;
        std::stringstream ss(pathStr);
        std::string dirToken;
        while (std::getline(ss, dirToken, ':')) {
            if (!dirToken.empty()) {
                searchDirs.push_back(dirToken);
            }
        }

        for (const auto& dir : searchDirs) {
            std::error_code ec;
            if (!fs::exists(dir, ec) || !fs::is_directory(dir, ec)) {
                continue;
            }

            try {
                for (const auto& entry : fs::directory_iterator(dir, fs::directory_options::skip_permission_denied, ec)) {
                    if (ec) continue;

                    std::string filename = entry.path().filename().string();
                    if (filename.empty() || filename[0] == '.') {
                        continue;
                    }

                    if (seen.find(filename) != seen.end()) {
                        continue;
                    }

                    // Check if regular file or symlink
                    bool isExec = false;
                    try {
                        auto status = entry.status();
                        if (fs::is_regular_file(status) || fs::is_symlink(entry.symlink_status())) {
                            auto perms = status.permissions();
                            if ((perms & (fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec)) != fs::perms::none) {
                                isExec = true;
                            }
                        }
                    } catch (...) {
                        continue;
                    }

                    if (!isExec) {
                        continue;
                    }

                    seen.insert(filename);
                    items.push_back(ItemFactory::makeRun(filename, entry.path().string(), nullptr, "system-run"));
                }
            } catch (...) {
                continue;
            }
        }

        // Sort items alphabetically by command name
        std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) {
            return a.displayName() < b.displayName();
        });

        s_cachedExecutables = items;
        s_hasCached = true;

        return s_cachedExecutables;
    }
};

} // namespace HLMenu
