#pragma once

#include "core/Config.hpp"
#include "fs/MountManager.hpp"
#include "core/Process.hpp"
#include "fs/FileOps.hpp"
#include "core/Logger.hpp"

namespace vaxp {

class StepContext {
public:
    StepContext(const BuildConfig& cfg, MountManager& mm)
        : config_(cfg), mount_manager_(mm) {
        rootfs_path_   = config_.get_absolute_rootfs();
        image_path_    = config_.get_absolute_image();
        dist_path_     = config_.get_absolute_dist();
        workspace_root_ = config_.workspace_root;
    }

    [[nodiscard]] const BuildConfig& config() const noexcept { return config_; }
    [[nodiscard]] MountManager& mounts() noexcept { return mount_manager_; }

    [[nodiscard]] fs::path assets_path() const noexcept { return config_.get_absolute_assets(); }
    [[nodiscard]] const fs::path& rootfs_path() const noexcept { return rootfs_path_; }
    [[nodiscard]] const fs::path& image_path() const noexcept { return image_path_; }
    [[nodiscard]] const fs::path& dist_path() const noexcept { return dist_path_; }
    [[nodiscard]] const fs::path& workspace_root() const noexcept { return workspace_root_; }

    // Helpers to run host / chroot commands
    ProcessResult run_host(const std::string& cmd, 
                           const std::vector<std::string>& args = {},
                           const std::string& prefix = "") const {
        ProcessOptions opts;
        opts.stream_output = true;
        opts.log_prefix = prefix.empty() ? cmd : prefix;
        return Process::run(cmd, args, opts);
    }

    ProcessResult run_host_shell(const std::string& shell_cmd,
                                 const std::string& prefix = "") const {
        ProcessOptions opts;
        opts.stream_output = true;
        opts.log_prefix = prefix;
        return Process::run_shell(shell_cmd, opts);
    }

    ProcessResult run_chroot(const std::string& cmd,
                             const std::vector<std::string>& args = {},
                             const std::map<std::string, std::string>& env = {},
                             const std::string& prefix = "") const {
        ProcessOptions opts;
        opts.stream_output = true;
        opts.log_prefix = prefix.empty() ? ("chroot:" + cmd) : prefix;
        opts.environment = env;
        opts.environment["DEBIAN_FRONTEND"] = "noninteractive";
        opts.environment["HOME"] = "/root";
        return Process::run_in_chroot(rootfs_path_, cmd, args, opts);
    }

    ProcessResult run_chroot_shell(const std::string& shell_cmd,
                                   const std::map<std::string, std::string>& env = {},
                                   const std::string& prefix = "") const {
        ProcessOptions opts;
        opts.stream_output = true;
        opts.log_prefix = prefix.empty() ? "chroot" : prefix;
        opts.environment = env;
        opts.environment["DEBIAN_FRONTEND"] = "noninteractive";
        opts.environment["HOME"] = "/root";
        return Process::run_shell_in_chroot(rootfs_path_, shell_cmd, opts);
    }

    // Direct rootfs file helpers
    bool write_rootfs_file(const fs::path& rel_path, const std::string& content, mode_t mode = 0644) const {
        fs::path full_path = rootfs_path_ / (rel_path.is_relative() ? rel_path : rel_path.relative_path());
        return FileOps::write_file_atomic(full_path, content, mode);
    }

    std::string read_rootfs_file(const fs::path& rel_path) const {
        fs::path full_path = rootfs_path_ / (rel_path.is_relative() ? rel_path : rel_path.relative_path());
        return FileOps::read_file(full_path);
    }

    bool copy_to_rootfs(const fs::path& host_src, const fs::path& rootfs_rel_dst) const {
        fs::path full_dst = rootfs_path_ / (rootfs_rel_dst.is_relative() ? rootfs_rel_dst : rootfs_rel_dst.relative_path());
        return FileOps::copy_directory_recursive(host_src, full_dst);
    }

private:
    const BuildConfig& config_;
    MountManager& mount_manager_;
    fs::path rootfs_path_;
    fs::path image_path_;
    fs::path dist_path_;
    fs::path workspace_root_;
};

} // namespace vaxp
