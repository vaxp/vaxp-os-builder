#include "engine/BootstrapEngine.hpp"
#include "core/Logger.hpp"
#include "core/Process.hpp"
#include "fs/FileOps.hpp"

namespace vaxp {

bool BootstrapEngine::is_bootstrapped(const fs::path& rootfs_path) {
    return fs::exists(rootfs_path / "bin") && 
           fs::exists(rootfs_path / "etc") && 
           fs::exists(rootfs_path / "usr");
}

bool BootstrapEngine::bootstrap(StepContext& ctx) {
    const auto& rootfs = ctx.rootfs_path();
    const auto& cfg    = ctx.config();

    if (is_bootstrapped(rootfs)) {
        LOG_OK("Base system already exists in " + rootfs.string() + ". Skipping debootstrap for resume.");
        return true;
    }

    LOG_INFO("Bootstrapping minimal base Debian/Ubuntu system (" + cfg.ubuntu_codename + 
             ", " + cfg.architecture + ") from " + cfg.build_mirror + "...");

    FileOps::ensure_directory_exists(rootfs);

    std::vector<std::string> args = {
        "--arch=" + cfg.architecture,
        "--variant=minbase",
        cfg.ubuntu_codename,
        rootfs.string(),
        cfg.build_mirror
    };

    ProcessOptions opts;
    opts.stream_output = true;
    opts.log_prefix = "debootstrap";

    auto res = Process::run("debootstrap", args, opts);
    if (!res.success()) {
        LOG_ERROR("debootstrap failed with return code: " + std::to_string(res.exit_code));
        return false;
    }

    if (!is_bootstrapped(rootfs)) {
        LOG_ERROR("debootstrap exited with success but critical directories are missing!");
        return false;
    }

    LOG_OK("Base system bootstrap completed successfully.");
    return true;
}

} // namespace vaxp
