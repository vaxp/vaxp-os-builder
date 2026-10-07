#include "steps/BuildSteps.hpp"
#include "core/Logger.hpp"
#include "fs/FileOps.hpp"
#include <unistd.h>
#include <random>
#include <sstream>
#include <iomanip>

namespace vaxp {

// -----------------------------------------------------------------------------
// 00 - Check Host
// -----------------------------------------------------------------------------
bool CheckHostStep::execute(StepContext& ctx) {
    LOG_INFO("Validating host system environment...");

    if (geteuid() != 0) {
        LOG_ERROR("VAXP-OS Builder must be run with root privileges (e.g. sudo vaxp-builder).");
        return false;
    }
    LOG_OK("Root user privileges confirmed.");

    struct ToolDep {
        std::string command;
        std::string package_name;
    };

    const std::vector<ToolDep> required_tools = {
        {"debootstrap", "debootstrap"},
        {"xorriso", "xorriso"},
        {"mksquashfs", "squashfs-tools"},
        {"unsquashfs", "squashfs-tools"},
        {"grub-mkstandalone", "grub2-common"},
        {"grub-install", "grub2-common"},
        {"mkfs.vfat", "dosfstools"},
        {"mcopy", "mtools"},
        {"dd", "coreutils"}
    };

    for (const auto& tool : required_tools) {
        auto res = ctx.run_host("which", {tool.command});
        if (!res.success()) {
            LOG_ERROR("Required host tool missing from PATH: " + tool.command +
                      " (Install package: " + tool.package_name + ")");
            return false;
        }
    }

    // Verify GRUB target platform boot assets
    if (!fs::exists("/usr/lib/grub/i386-pc/cdboot.img")) {
        LOG_ERROR("Required GRUB BIOS module missing: /usr/lib/grub/i386-pc/cdboot.img (Install package: grub-pc-bin)");
        return false;
    }
    if (!fs::is_directory("/usr/lib/grub/x86_64-efi")) {
        LOG_ERROR("Required GRUB EFI directory missing: /usr/lib/grub/x86_64-efi (Install package: grub-efi-amd64)");
        return false;
    }

    LOG_OK("All required build utilities and GRUB boot assets are verified on host.");

    uint64_t available_bytes = FileOps::get_available_disk_space(ctx.workspace_root());
    double gb_avail = static_cast<double>(available_bytes) / (1024.0 * 1024.0 * 1024.0);
    LOG_INFO("Available disk space: " + std::to_string(gb_avail) + " GB");

    return true;
}

// -----------------------------------------------------------------------------
// 01 - APT Source
// -----------------------------------------------------------------------------
bool AptSourceStep::execute(StepContext& ctx) {
    const auto& cfg = ctx.config();
    LOG_INFO("Configuring build APT sources for: " + cfg.ubuntu_codename + " (" + cfg.build_mirror + ")");

    std::string sources_list =
        "deb " + cfg.build_mirror + " " + cfg.ubuntu_codename + " main restricted universe multiverse\n"
        "deb " + cfg.build_mirror + " " + cfg.ubuntu_codename + "-updates main restricted universe multiverse\n"
        "deb " + cfg.build_mirror + " " + cfg.ubuntu_codename + "-backports main restricted universe multiverse\n"
        "deb " + cfg.build_mirror + " " + cfg.ubuntu_codename + "-security main restricted universe multiverse\n";

    if (!ctx.write_rootfs_file("etc/apt/sources.list", sources_list)) {
        LOG_ERROR("Failed to write etc/apt/sources.list in rootfs.");
        return false;
    }

    LOG_OK("APT build sources configured.");
    return true;
}

// -----------------------------------------------------------------------------
// 02 - Set Hostname & Base Locales
// -----------------------------------------------------------------------------
bool SetHostnameStep::execute(StepContext& ctx) {
    const auto& cfg = ctx.config();
    LOG_INFO("Configuring hostname to: " + cfg.target_name);

    ctx.write_rootfs_file("etc/hostname", cfg.target_name + "\n");

    std::string hosts_content = 
        "127.0.0.1 localhost\n"
        "127.0.1.1 " + cfg.target_name + "\n\n"
        "::1     ip6-localhost ip6-loopback\n"
        "fe00::0 ip6-localnet\n"
        "ff00::0 ip6-mcastprefix\n"
        "ff02::1 ip6-allnodes\n"
        "ff02::2 ip6-allrouters\n";
    ctx.write_rootfs_file("etc/hosts", hosts_content);

    LOG_INFO("Updating package database and installing locales, resolvconf, apt-utils...");
    ctx.run_chroot("apt-get", {"update"});
    ctx.run_chroot("apt-get", {"install", "-y", "--no-install-recommends", "locales", "resolvconf", "apt-utils"});

    std::string locale_str = cfg.lang_mode + ".UTF-8";
    std::string locale_gen = locale_str + " UTF-8\n";
    if (cfg.lang_mode != "en_US") {
        locale_gen += "en_US.UTF-8 UTF-8\n";
    }
    ctx.write_rootfs_file("etc/locale.gen", locale_gen);

    ctx.run_chroot("locale-gen");
    ctx.run_chroot("update-locale", {"LANG=" + locale_str, "LC_ALL=" + locale_str});

    LOG_OK("Hostname and initial localization configured.");
    return true;
}

// -----------------------------------------------------------------------------
// 03 - Systemd Subsystem
// -----------------------------------------------------------------------------
bool SystemdStep::execute(StepContext& ctx) {
    LOG_INFO("Installing systemd and core daemons...");

    std::vector<std::string> pkgs = {
        "install", "-y", "--no-install-recommends",
        "systemd-sysv",
        "libterm-readline-gnu-perl",
        "wget",
        "krb5-locales",
        "publicsuffix",
        "libnss-systemd",
        "networkd-dispatcher",
        "shared-mime-info",
        "dmsetup",
        "xdg-user-dirs",
        "ca-certificates"
    };

    auto res = ctx.run_chroot("apt-get", pkgs);
    if (!res.success()) {
        LOG_ERROR("Failed to install systemd-sysv core packages.");
        return false;
    }

    LOG_OK("Systemd subsystem installed.");
    return true;
}

// -----------------------------------------------------------------------------
// 04 - Machine ID
// -----------------------------------------------------------------------------
bool MachineIdStep::execute(StepContext& ctx) {
    LOG_INFO("Initializing temporary machine-id...");

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dis(0, 0xFFFFFFFF);

    std::stringstream ss;
    for (int i = 0; i < 4; ++i) {
        ss << std::hex << std::setw(8) << std::setfill('0') << dis(gen);
    }
    std::string mid = ss.str();

    ctx.write_rootfs_file("etc/machine-id", mid + "\n");
    FileOps::ensure_directory_exists(ctx.rootfs_path() / "var/lib/dbus");
    ctx.run_chroot_shell("ln -fs /etc/machine-id /var/lib/dbus/machine-id");

    LOG_OK("Machine-id initialized.");
    return true;
}

// -----------------------------------------------------------------------------
// 05 - Initctl Diversion
// -----------------------------------------------------------------------------
bool InitctlStep::execute(StepContext& ctx) {
    LOG_INFO("Configuring initctl diversion...");
    ctx.run_chroot("dpkg-divert", {"--local", "--rename", "--add", "/sbin/initctl"});
    ctx.run_chroot_shell("ln -sf /bin/true /sbin/initctl");
    LOG_OK("Initctl diversion configured.");
    return true;
}

// -----------------------------------------------------------------------------
// 06 - APT Upgrade
// -----------------------------------------------------------------------------
bool AptUpgradeStep::execute(StepContext& ctx) {
    LOG_INFO("Upgrading base packages in rootfs...");
    auto res = ctx.run_chroot("apt-get", {"-y", "upgrade"});
    if (!res.success()) {
        LOG_WARN("apt-get upgrade returned warning code.");
    }
    LOG_OK("Base packages upgraded.");
    return true;
}

// -----------------------------------------------------------------------------
// 07 - System Tools & Drivers
// -----------------------------------------------------------------------------
bool SystemToolsInstallStep::execute(StepContext& ctx) {
    LOG_INFO("Installing system tools and graphics drivers...");

    std::vector<std::string> tools = {
        "install", "-y", "--no-install-recommends",
        "apparmor", "bash-completion", "bind9-dnsutils", "bolt", "busybox-static",
        "coreutils", "cpio", "crash", "cron", "debconf-i18n", "dmidecode", "dosfstools",
        "ed", "ethtool", "fdisk", "file", "firmware-sof-signed", "ftp", "grub-common",
        "grub2-common", "hdparm", "hwdata", "init", "iproute2", "iptables",
        "libpam-systemd", "libpam-cap", "libpam-fprintd", "libpam-modules",
        "libpam-modules-bin", "libpam-pwquality", "libpam-sss", "linux-firmware",
        "locales", "logrotate", "lshw", "lsof", "man-db", "manpages", "manpages-dev",
        "dns-root-data", "usb-modeswitch", "libmbim-utils", "media-types", "mtr-tiny",
        "network-manager", "nftables", "numactl", "openssh-client", "python3-systemd",
        "parted", "pciutils", "psmisc", "resolvconf", "rsync", "strace", "sudo",
        "tcpdump", "telnet", "time", "ufw", "unzip", "usbutils", "uuid-runtime",
        "wget", "xz-utils", "zstd", "zip", "powermgmt-base", "dbus-user-session",
        "dnsmasq-base", "wpasupplicant", "python3-rich", "systemd-hwe-hwdb",
        "efibootmgr", "ibverbs-providers", "xauth", "busybox-initramfs", "dhcpcd-base",
        "kmod", "linux-base", "cifs-utils", "eject", "gettext", "cracklib-runtime",
        "libfuse2t64", "libfuse3-3", "libopengl0", "initramfs-tools",
        // GPU & Acceleration Drivers
        "i965-va-driver-shaders", "mesa-vdpau-drivers", "mesa-va-drivers",
        "mesa-vulkan-drivers", "intel-media-va-driver-non-free", "libgl1-mesa-dri",
        "intel-opencl-icd", "mesa-opencl-icd"
    };

    auto res = ctx.run_chroot("apt-get", tools);
    if (!res.success()) {
        LOG_ERROR("Failed to install base system tools and graphics drivers.");
        return false;
    }

    // Pin base-files
    LOG_INFO("Pinning base-files package to hold...");
    ctx.run_chroot("apt-mark", {"hold", "base-files"});
    std::string pin_pref =
        "Package: base-files\n"
        "Pin: release o=Ubuntu\n"
        "Pin-Priority: -1\n";
    ctx.write_rootfs_file("etc/apt/preferences.d/no-upgrade-base-files", pin_pref);

    LOG_OK("Hardware stack and system tools installed.");
    return true;
}

// -----------------------------------------------------------------------------
// 08 - Casper & Kernel
// -----------------------------------------------------------------------------
bool CasperKernelInstallStep::execute(StepContext& ctx) {
    LOG_INFO("Installing Casper live environment and Linux Kernel...");

    std::vector<std::string> casper_pkgs = {
        "install", "-y", "--no-install-recommends",
        "casper", "discover", "laptop-detect", "os-prober", "keyutils", "thermald"
    };

    auto ksearch = ctx.run_chroot_shell("apt-cache search 'linux-generic-hwe-' | awk '{print $1}' | sort -V | tail -n 1");
    std::string kernel_pkg = "linux-generic";
    if (ksearch.success() && !ksearch.stdout_output.empty()) {
        std::stringstream ss(ksearch.stdout_output);
        std::string cand;
        ss >> cand;
        if (!cand.empty()) kernel_pkg = cand;
    }
    LOG_INFO("Selected kernel: " + kernel_pkg);
    casper_pkgs.push_back(kernel_pkg);

    auto res = ctx.run_chroot("apt-get", casper_pkgs);
    if (!res.success()) {
        LOG_ERROR("Failed to install Casper and Linux kernel.");
        return false;
    }

    LOG_OK("Casper and Linux kernel successfully installed.");
    return true;
}

} // namespace vaxp
