#pragma once

#include "core/Types.hpp"
#include <string>
#include <vector>
#include <filesystem>
#include <nlohmann/json.hpp>

namespace vaxp {

struct BuildConfig {
    // OS Identification
    std::string target_name{"vaxp-os"};
    std::string business_name{"VAXP-OS"};
    std::string version{"0.1.0"};
    std::string ubuntu_codename{"noble"};
    std::string architecture{"amd64"};

    // Mirrors
    std::string build_mirror{"http://archive.ubuntu.com/ubuntu/"};
    std::string live_mirror{"http://archive.ubuntu.com/ubuntu/"};

    // Regional & Localization
    std::string lang_mode{"en_US"};
    std::string lang_pack_code{"en"};
    std::string timezone{"America/Los_Angeles"};

    // Store & Features
    StoreProvider store_provider{StoreProvider::WEB};
    std::string kernel_package_pattern{"linux-generic-hwe-*"};

    // Directories (Absolute or relative to workspace root)
    fs::path workspace_root{"."};
    fs::path assets_dir{"assets"};
    fs::path rootfs_dir{"src/new_building_os"};
    fs::path image_dir{"src/image"};
    fs::path dist_dir{"src/dist"};
    fs::path mods_dir{"src/mods"};

    // Packages customization
    std::vector<std::string> packages_to_remove{
        "ubiquity",
        "casper",
        "discover",
        "laptop-detect",
        "os-prober"
    };

    std::vector<std::string> default_apps{
        "ffmpegthumbnailer",
        "libgdk-pixbuf2.0-bin",
        "usb-creator-gtk",
        "policykit-desktop-privileges"
    };

    std::vector<std::string> default_cli_tools{
        "curl",
        "vim",
        "nano",
        "git",
        "build-essential",
        "make",
        "gcc",
        "g++",
        "dpkg-dev",
        "net-tools",
        "htop",
        "httping",
        "iputils-ping",
        "iputils-tracepath",
        "dnsutils",
        "smartmontools",
        "traceroute",
        "whois"
    };

    // CLI Execution Modes
    bool resume{true};
    bool dry_run{false};
    bool verbose{false};
    bool force_clean{false};
    std::string single_step_id;

    // Helpers
    [[nodiscard]] fs::path get_absolute_assets() const;
    [[nodiscard]] fs::path get_absolute_rootfs() const;
    [[nodiscard]] fs::path get_absolute_image() const;
    [[nodiscard]] fs::path get_absolute_dist() const;
    [[nodiscard]] fs::path get_absolute_mods() const;

    // JSON serialization
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);

    static BuildConfig load_from_file(const fs::path& path);
    void save_to_file(const fs::path& path) const;
    
    bool parse_cli(int argc, char* argv[]);
    static void print_usage(const char* prog_name);
};

} // namespace vaxp
