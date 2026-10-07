#include "fs/MountManager.hpp"
#include "core/Logger.hpp"
#include "core/SignalHandler.hpp"
#include "fs/FileOps.hpp"

#include <sys/mount.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <algorithm>

namespace vaxp {

MountManager& MountManager::instance() {
    static MountManager mm;
    return mm;
}

MountManager::MountManager() {
    // Register global emergency cleanup with SignalHandler
    SignalHandler::instance().register_cleanup([this]() {
        std::lock_guard<std::mutex> lock(this->mutex_);
        for (auto it = this->active_mount_paths_.rbegin(); it != this->active_mount_paths_.rend(); ++it) {
            umount2(it->c_str(), MNT_DETACH);
        }
    });
}

MountManager::~MountManager() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = active_mount_paths_.rbegin(); it != active_mount_paths_.rend(); ++it) {
        umount2(it->c_str(), MNT_DETACH);
    }
    active_mount_paths_.clear();
}

bool MountManager::is_mounted(const fs::path& target) const {
    return std::find(active_mount_paths_.begin(), active_mount_paths_.end(), target) != active_mount_paths_.end();
}

bool MountManager::mount_bind(const fs::path& source, const fs::path& target) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (is_mounted(target)) {
        return true;
    }

    FileOps::ensure_directory_exists(target);

    if (mount(source.c_str(), target.c_str(), nullptr, MS_BIND, nullptr) != 0) {
        LOG_ERROR("mount bind failed from " + source.string() + " to " + target.string() + ": " + strerror(errno));
        return false;
    }

    active_mount_paths_.push_back(target);
    LOG_DEBUG("Mounted bind: " + source.string() + " -> " + target.string());
    return true;
}

bool MountManager::mount_special(const std::string& source, const fs::path& target, 
                                 const std::string& fstype, unsigned long flags) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (is_mounted(target)) {
        return true;
    }

    FileOps::ensure_directory_exists(target);

    if (mount(source.c_str(), target.c_str(), fstype.c_str(), flags, nullptr) != 0) {
        LOG_ERROR("mount special failed (" + fstype + ") at " + target.string() + ": " + strerror(errno));
        return false;
    }

    active_mount_paths_.push_back(target);
    LOG_DEBUG("Mounted special (" + fstype + ") at " + target.string());
    return true;
}

bool MountManager::unmount(const fs::path& target, bool lazy) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    int flags = lazy ? MNT_DETACH : 0;
    int res = umount2(target.c_str(), flags);
    if (res != 0 && errno != EINVAL) { // EINVAL means not a mountpoint
        LOG_WARN("umount2 failed on " + target.string() + ": " + strerror(errno));
    }

    auto it = std::find(active_mount_paths_.begin(), active_mount_paths_.end(), target);
    if (it != active_mount_paths_.end()) {
        active_mount_paths_.erase(it);
    }
    return true;
}

bool MountManager::mount_all(const fs::path& rootfs_path) {
    LOG_INFO("Setting up virtual filesystems in chroot environment: " + rootfs_path.string());

    fs::path dev_path  = rootfs_path / "dev";
    fs::path run_path  = rootfs_path / "run";
    fs::path proc_path = rootfs_path / "proc";
    fs::path sys_path  = rootfs_path / "sys";
    fs::path pts_path  = rootfs_path / "dev/pts";

    if (!mount_bind("/dev", dev_path)) return false;
    if (!mount_bind("/run", run_path)) return false;
    if (!mount_special("proc", proc_path, "proc")) return false;
    if (!mount_special("sysfs", sys_path, "sysfs")) return false;
    if (!mount_special("devpts", pts_path, "devpts", MS_NOSUID | MS_NOEXEC)) return false;

    LOG_OK("Virtual filesystems successfully mounted.");
    return true;
}

bool MountManager::unmount_all(const fs::path& rootfs_path) {
    LOG_INFO("Unmounting virtual filesystems from chroot environment: " + rootfs_path.string());

    fs::path pts_path  = rootfs_path / "dev/pts";
    fs::path sys_path  = rootfs_path / "sys";
    fs::path proc_path = rootfs_path / "proc";
    fs::path run_path  = rootfs_path / "run";
    fs::path dev_path  = rootfs_path / "dev";

    // Unmount in reverse order
    unmount(pts_path);
    unmount(sys_path);
    unmount(proc_path);
    unmount(run_path);
    unmount(dev_path);

    LOG_OK("Chroot unmount completed.");
    return true;
}

} // namespace vaxp
