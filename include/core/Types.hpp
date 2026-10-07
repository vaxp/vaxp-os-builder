#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <optional>
#include <chrono>
#include <functional>

namespace vaxp {

namespace fs = std::filesystem;

enum class LogLevel {
    DEBUG,
    INFO,
    SUCCESS,
    WARN,
    ERROR,
    STEP
};

enum class StoreProvider {
    NONE,
    WEB,
    FLATPAK,
    SNAP
};

inline std::string store_provider_to_string(StoreProvider sp) {
    switch (sp) {
        case StoreProvider::NONE: return "none";
        case StoreProvider::WEB: return "web";
        case StoreProvider::FLATPAK: return "flatpak";
        case StoreProvider::SNAP: return "snap";
    }
    return "web";
}

inline StoreProvider store_provider_from_string(const std::string& str) {
    if (str == "none") return StoreProvider::NONE;
    if (str == "flatpak") return StoreProvider::FLATPAK;
    if (str == "snap") return StoreProvider::SNAP;
    return StoreProvider::WEB;
}

struct ProcessResult {
    int exit_code{-1};
    std::string stdout_output;
    std::string stderr_output;

    [[nodiscard]] bool success() const noexcept {
        return exit_code == 0;
    }
};

} // namespace vaxp
