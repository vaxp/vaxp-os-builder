#pragma once

#include "core/Types.hpp"
#include <vector>
#include <string>
#include <filesystem>
#include <mutex>

namespace vaxp {

struct MountPoint {
    fs::path source;
    fs::path target;
    std::string fs_type;
    unsigned long flags{0};
    bool is_bind{false};
};

class MountManager {
public:
    static MountManager& instance();

    ~MountManager();

    bool mount_all(const fs::path& rootfs_path);
    bool unmount_all(const fs::path& rootfs_path);

    bool mount_bind(const fs::path& source, const fs::path& target);
    bool mount_special(const std::string& source, const fs::path& target, 
                       const std::string& fstype, unsigned long flags = 0);

    bool unmount(const fs::path& target, bool lazy = true);

    [[nodiscard]] bool is_mounted(const fs::path& target) const;
    [[nodiscard]] const std::vector<fs::path>& active_mounts() const { return active_mount_paths_; }

private:
    MountManager();
    MountManager(const MountManager&) = delete;
    MountManager& operator=(const MountManager&) = delete;

    std::mutex mutex_;
    std::vector<fs::path> active_mount_paths_;
};

} // namespace vaxp
