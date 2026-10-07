#include "core/Config.hpp"
#include "core/Logger.hpp"
#include <fstream>
#include <iostream>

namespace vaxp {

fs::path BuildConfig::get_absolute_assets() const {
    if (assets_dir.is_absolute()) return assets_dir;
    // Check if assets folder is inside VAXP-OS-Builder/assets
    if (fs::exists(workspace_root / "VAXP-OS-Builder" / assets_dir)) {
        return workspace_root / "VAXP-OS-Builder" / assets_dir;
    }
    return workspace_root / assets_dir;
}

fs::path BuildConfig::get_absolute_rootfs() const {
    if (rootfs_dir.is_absolute()) return rootfs_dir;
    return workspace_root / rootfs_dir;
}

fs::path BuildConfig::get_absolute_image() const {
    if (image_dir.is_absolute()) return image_dir;
    return workspace_root / image_dir;
}

fs::path BuildConfig::get_absolute_dist() const {
    if (dist_dir.is_absolute()) return dist_dir;
    return workspace_root / dist_dir;
}

fs::path BuildConfig::get_absolute_mods() const {
    if (mods_dir.is_absolute()) return mods_dir;
    return workspace_root / mods_dir;
}

nlohmann::json BuildConfig::to_json() const {
    nlohmann::json j;
    j["target_name"] = target_name;
    j["business_name"] = business_name;
    j["version"] = version;
    j["ubuntu_codename"] = ubuntu_codename;
    j["architecture"] = architecture;
    j["build_mirror"] = build_mirror;
    j["live_mirror"] = live_mirror;
    j["lang_mode"] = lang_mode;
    j["lang_pack_code"] = lang_pack_code;
    j["timezone"] = timezone;
    j["store_provider"] = store_provider_to_string(store_provider);
    j["workspace_root"] = workspace_root.string();
    j["assets_dir"] = assets_dir.string();
    j["rootfs_dir"] = rootfs_dir.string();
    j["image_dir"] = image_dir.string();
    j["dist_dir"] = dist_dir.string();
    j["mods_dir"] = mods_dir.string();
    j["packages_to_remove"] = packages_to_remove;
    j["default_apps"] = default_apps;
    j["default_cli_tools"] = default_cli_tools;
    return j;
}

void BuildConfig::from_json(const nlohmann::json& j) {
    if (j.contains("target_name")) target_name = j["target_name"].get<std::string>();
    if (j.contains("business_name")) business_name = j["business_name"].get<std::string>();
    if (j.contains("version")) version = j["version"].get<std::string>();
    if (j.contains("ubuntu_codename")) ubuntu_codename = j["ubuntu_codename"].get<std::string>();
    if (j.contains("architecture")) architecture = j["architecture"].get<std::string>();
    if (j.contains("build_mirror")) build_mirror = j["build_mirror"].get<std::string>();
    if (j.contains("live_mirror")) live_mirror = j["live_mirror"].get<std::string>();
    if (j.contains("lang_mode")) lang_mode = j["lang_mode"].get<std::string>();
    if (j.contains("lang_pack_code")) lang_pack_code = j["lang_pack_code"].get<std::string>();
    if (j.contains("timezone")) timezone = j["timezone"].get<std::string>();
    if (j.contains("store_provider")) {
        store_provider = store_provider_from_string(j["store_provider"].get<std::string>());
    }
    if (j.contains("workspace_root")) workspace_root = j["workspace_root"].get<std::string>();
    if (j.contains("assets_dir")) assets_dir = j["assets_dir"].get<std::string>();
    if (j.contains("rootfs_dir")) rootfs_dir = j["rootfs_dir"].get<std::string>();
    if (j.contains("image_dir")) image_dir = j["image_dir"].get<std::string>();
    if (j.contains("dist_dir")) dist_dir = j["dist_dir"].get<std::string>();
    if (j.contains("mods_dir")) mods_dir = j["mods_dir"].get<std::string>();
    if (j.contains("packages_to_remove")) {
        packages_to_remove = j["packages_to_remove"].get<std::vector<std::string>>();
    }
    if (j.contains("default_apps")) {
        default_apps = j["default_apps"].get<std::vector<std::string>>();
    }
    if (j.contains("default_cli_tools")) {
        default_cli_tools = j["default_cli_tools"].get<std::vector<std::string>>();
    }
}

BuildConfig BuildConfig::load_from_file(const fs::path& path) {
    BuildConfig config;
    std::ifstream file(path);
    if (!file.is_open()) {
        LOG_WARN("Could not open config file: " + path.string() + ", using default configuration.");
        return config;
    }
    try {
        nlohmann::json j;
        file >> j;
        config.from_json(j);
        LOG_INFO("Loaded configuration from: " + path.string());
    } catch (const std::exception& e) {
        LOG_ERROR("JSON parsing error in config: " + std::string(e.what()));
    }
    return config;
}

void BuildConfig::save_to_file(const fs::path& path) const {
    std::ofstream file(path);
    if (!file.is_open()) {
        LOG_ERROR("Cannot write config to: " + path.string());
        return;
    }
    file << to_json().dump(4) << "\n";
}

void BuildConfig::print_usage(const char* prog_name) {
    std::cout << "VAXP-OS Builder - High Performance OS Distribution Engine\n"
              << "Usage: " << prog_name << " [options]\n\n"
              << "Options:\n"
              << "  -c, --config <file>     Specify JSON configuration file\n"
              << "  -s, --step <id>         Run only a specific step by its ID\n"
              << "  -l, --list-steps        List all registered build pipeline steps\n"
              << "  --no-resume             Do not resume from previous build state (start fresh)\n"
              << "  --clean                 Clean up build workspace (mounts, rootfs, images)\n"
              << "  --dry-run               Simulate build without executing intrusive changes\n"
              << "  -v, --verbose           Enable verbose output\n"
              << "  -w, --workspace <dir>   Set workspace root directory (default: current)\n"
              << "  -h, --help              Show this help message\n";
}

bool BuildConfig::parse_cli(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return false;
        } else if (arg == "-c" || arg == "--config") {
            if (i + 1 < argc) {
                fs::path cfg_path = argv[++i];
                *this = load_from_file(cfg_path);
            } else {
                LOG_ERROR("--config requires a file argument.");
                return false;
            }
        } else if (arg == "-s" || arg == "--step") {
            if (i + 1 < argc) {
                single_step_id = argv[++i];
            } else {
                LOG_ERROR("--step requires a step ID argument.");
                return false;
            }
        } else if (arg == "-w" || arg == "--workspace") {
            if (i + 1 < argc) {
                workspace_root = argv[++i];
            } else {
                LOG_ERROR("--workspace requires a directory argument.");
                return false;
            }
        } else if (arg == "--no-resume") {
            resume = false;
        } else if (arg == "--clean") {
            force_clean = true;
        } else if (arg == "--dry-run") {
            dry_run = true;
        } else if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        }
    }
    return true;
}

} // namespace vaxp
