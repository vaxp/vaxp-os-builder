#include "steps/BuildSteps.hpp"
#include "core/Logger.hpp"
#include "fs/FileOps.hpp"
#include <iostream>
#include <poll.h>
#include <unistd.h>

namespace vaxp {

// -----------------------------------------------------------------------------
// 40 - Extension Package Artifacts
// -----------------------------------------------------------------------------
bool CopPkgeStep::execute(StepContext& ctx) {
    LOG_INFO("Processing extension package artifacts...");
    FileOps::ensure_directory_exists(ctx.rootfs_path() / "tmp");
    return true;
}

// -----------------------------------------------------------------------------
// 40.9 - Interactive Manual Control Prompt
// -----------------------------------------------------------------------------
bool ManualControlStep::execute(StepContext& ctx) {
    LOG_INFO("Prompting user for Manual Chroot Control...");

    std::cout << "\n=========================================================\n"
              << "       التحكم اليدوي التفاعلي - Manual Chroot Mode\n"
              << "  هل تريد الدخول إلى بيئة chroot لتنفيذ أوامر يدوية أو خدمات خاصة؟\n"
              << "  Enter 'y' for Manual Control, or wait 10s to continue [y/N]: \n"
              << "=========================================================\n";
    std::cout.flush();

    struct pollfd pfd{};
    pfd.fd = STDIN_FILENO;
    pfd.events = POLLIN;

    int ret = poll(&pfd, 1, 10000); // 10 seconds timeout
    if (ret > 0 && (pfd.revents & POLLIN)) {
        std::string choice;
        std::getline(std::cin, choice);
        if (choice == "y" || choice == "Y") {
            LOG_INFO("User activated manual chroot session. Spawning interactive bash shell...");
            std::cout << "\n=== Entered Chroot Environment (Type 'exit' to resume build) ===\n";
            ctx.run_chroot_shell("/bin/bash");
            std::cout << "=== Exited Chroot Environment. Resuming build pipeline ===\n\n";
            LOG_OK("Manual chroot session concluded.");
            return true;
        }
    }

    LOG_INFO("Continuing automatically without manual intervention.");
    return true;
}

// -----------------------------------------------------------------------------
// 41 - Target APT Live Mirror
// -----------------------------------------------------------------------------
bool TargetAptMirrorStep::execute(StepContext& ctx) {
    const auto& cfg = ctx.config();
    LOG_INFO("Configuring target live system APT repositories (" + cfg.live_mirror + ")...");

    std::string sources_list =
        "deb " + cfg.live_mirror + " " + cfg.ubuntu_codename + " main restricted universe multiverse\n"
        "deb " + cfg.live_mirror + " " + cfg.ubuntu_codename + "-updates main restricted universe multiverse\n"
        "deb " + cfg.live_mirror + " " + cfg.ubuntu_codename + "-backports main restricted universe multiverse\n"
        "deb " + cfg.live_mirror + " " + cfg.ubuntu_codename + "-security main restricted universe multiverse\n";

    ctx.write_rootfs_file("etc/apt/sources.list", sources_list);
    LOG_OK("Live APT sources configured.");
    return true;
}

// -----------------------------------------------------------------------------
// 42 - Intel SOF & ALSA UCM
// -----------------------------------------------------------------------------
bool IntelTheSofStep::execute(StepContext& ctx) {
    LOG_INFO("Deploying Sound Open Firmware (SOF) and ALSA UCM2 configurations...");

    std::string script =
        "if wget --spider -q --timeout=5 https://github.com/thesofproject/sof-bin/releases; then "
        "  TMPDIR=$(mktemp -d) && cd $TMPDIR && "
        "  wget -q https://github.com/thesofproject/sof-bin/releases/download/v2025.05/sof-bin-2025.05.tar.gz -O sof.tar.gz 2>/dev/null && "
        "  tar -xzf sof.tar.gz && rm -rf /lib/firmware/intel/sof* && "
        "  cd sof-bin-2025.05 && ./install.sh 2>/dev/null || true && "
        "  cd / && rm -rf $TMPDIR; "
        "fi";

    ctx.run_chroot_shell(script);
    LOG_OK("Audio firmware configuration completed.");
    return true;
}

// -----------------------------------------------------------------------------
// 42b - Sessions & AppArmor Security
// -----------------------------------------------------------------------------
bool SessionsPatchStep::execute(StepContext& ctx) {
    LOG_INFO("Configuring AppArmor unprivileged user namespaces policy...");

    std::string conf = "kernel.apparmor_restrict_unprivileged_userns = 0\n";
    ctx.write_rootfs_file("etc/sysctl.d/20-apparmor-donotrestrict.conf", conf);
    LOG_OK("AppArmor namespaces policy configured.");
    return true;
}

// -----------------------------------------------------------------------------
// 43 - Official Branding & Identity
// -----------------------------------------------------------------------------
bool EtcBrandingStep::execute(StepContext& ctx) {
    const auto& cfg = ctx.config();
    LOG_INFO("Applying official commercial branding for " + cfg.business_name + " " + cfg.version + "...");

    std::string lsb_release =
        "DISTRIB_ID=" + cfg.business_name + "\n"
        "DISTRIB_RELEASE=" + cfg.version + "\n"
        "DISTRIB_CODENAME=" + cfg.ubuntu_codename + "\n"
        "DISTRIB_DESCRIPTION=\"" + cfg.business_name + " " + cfg.version + "\"\n";
    ctx.write_rootfs_file("etc/lsb-release", lsb_release);

    std::string os_release =
        "PRETTY_NAME=\"" + cfg.business_name + " " + cfg.version + "\"\n"
        "NAME=\"" + cfg.business_name + "\"\n"
        "VERSION_ID=\"" + cfg.version + "\"\n"
        "VERSION=\"" + cfg.version + " (" + cfg.ubuntu_codename + ")\"\n"
        "VERSION_CODENAME=" + cfg.ubuntu_codename + "\n"
        "ID=vaxp\n"
        "ID_LIKE=\"ubuntu debian\"\n"
        "HOME_URL=\"https://www.vaxp.org/\"\n"
        "SUPPORT_URL=\"https://www.vaxp.org/\"\n"
        "BUG_REPORT_URL=\"https://www.vaxp.org/\"\n"
        "PRIVACY_POLICY_URL=\"https://www.vaxp.org/\"\n"
        "UBUNTU_CODENAME=" + cfg.ubuntu_codename + "\n";
    ctx.write_rootfs_file("etc/os-release", os_release);

    LOG_OK("OS branding and release descriptors deployed.");
    return true;
}

// -----------------------------------------------------------------------------
// 44 - Casper Configuration
// -----------------------------------------------------------------------------
bool CasperPatchStep::execute(StepContext& ctx) {
    const auto& cfg = ctx.config();
    LOG_INFO("Configuring Casper live boot session...");

    std::string casper_conf =
        "export USERNAME=\"live\"\n"
        "export USERFULLNAME=\"" + cfg.business_name + " Live session user\"\n"
        "export HOST=\"" + cfg.target_name + "\"\n"
        "export BUILD_SYSTEM=\"Ubuntu\"\n"
        "export FLAVOUR=\"" + cfg.business_name + "\"\n";

    ctx.write_rootfs_file("etc/casper.conf", casper_conf);
    LOG_OK("Casper live session parameters configured.");
    return true;
}

// -----------------------------------------------------------------------------
// 45 - Issue & Login Banners
// -----------------------------------------------------------------------------
bool EtcIssuePatchStep::execute(StepContext& ctx) {
    const auto& cfg = ctx.config();
    LOG_INFO("Configuring console login banners...");

    std::string issue = cfg.business_name + " " + cfg.version + " \\n \\l\n\n";
    ctx.write_rootfs_file("etc/issue", issue);
    ctx.write_rootfs_file("etc/issue.net", cfg.business_name + " " + cfg.version + "\n");

    LOG_OK("Login banners written.");
    return true;
}

// -----------------------------------------------------------------------------
// 78 - Disable Advertisements
// -----------------------------------------------------------------------------
bool NoAdvertisementsStep::execute(StepContext& ctx) {
    LOG_INFO("Verifying Ubuntu Pro advertisement hooks are removed...");
    FileOps::remove_all_safe(ctx.rootfs_path() / "etc/apt/apt.conf.d/20apt-esm-hook.conf");
    LOG_OK("Advertisements disabled.");
    return true;
}

// -----------------------------------------------------------------------------
// 79 - Purge Bloatware
// -----------------------------------------------------------------------------
bool UselessPackageRemoverStep::execute(StepContext& ctx) {
    LOG_INFO("Purging bloatware and unnecessary packages...");

    std::vector<std::string> bloatware = {
        "gnome-mahjongg", "gnome-mines", "gnome-sudoku", "aisleriot", "hitori",
        "gnome-initial-setup", "gnome-photos", "eog", "tilix", "gnome-contacts",
        "gnome-terminal", "zutty", "update-manager-core",
        "gnome-shell-extension-ubuntu-dock", "apport", "ubuntu-pro-client",
        "ubuntu-advantage-tools", "popularity-contest", "ubuntu-report", "whoopsie", "xterm"
    };

    for (const auto& pkg : bloatware) {
        ctx.run_chroot("apt-get", {"purge", "-y", pkg});
    }

    ctx.run_chroot("apt-get", {"autoremove", "-y", "--purge"});
    LOG_OK("Bloatware purged.");
    return true;
}

// -----------------------------------------------------------------------------
// 80 - Update Initramfs
// -----------------------------------------------------------------------------
bool InitramfsUpdateStep::execute(StepContext& ctx) {
    LOG_INFO("Rebuilding initramfs boot images inside chroot...");
    ctx.run_chroot("update-initramfs", {"-u", "-k", "all"});
    LOG_OK("Initramfs updated.");
    return true;
}

// -----------------------------------------------------------------------------
// 82 - Final Locales & Timezone
// -----------------------------------------------------------------------------
bool LocalesConfigStep::execute(StepContext& ctx) {
    const auto& cfg = ctx.config();
    LOG_INFO("Configuring final timezone (" + cfg.timezone + ") and locale...");

    fs::path zoneinfo = ctx.rootfs_path() / ("usr/share/zoneinfo/" + cfg.timezone);
    if (fs::exists(zoneinfo)) {
        ctx.write_rootfs_file("etc/timezone", cfg.timezone + "\n");
        FileOps::remove_all_safe(ctx.rootfs_path() / "etc/localtime");
        ctx.run_chroot_shell("ln -sf /usr/share/zoneinfo/" + cfg.timezone + " /etc/localtime");
    }

    std::string loc = "LANG=\"" + cfg.lang_mode + ".UTF-8\"\n";
    ctx.write_rootfs_file("etc/default/locale", loc);

    LOG_OK("Timezone and locale finalized.");
    return true;
}

// -----------------------------------------------------------------------------
// 83 - NetworkManager Configuration
// -----------------------------------------------------------------------------
bool NetworkManagerPatchStep::execute(StepContext& ctx) {
    LOG_INFO("Configuring NetworkManager and Netplan...");

    std::string nm_conf =
        "[main]\n"
        "rc-manager=resolvconf\n"
        "plugins=ifupdown,keyfile\n"
        "dns=dnsmasq\n\n"
        "[ifupdown]\n"
        "managed=false\n";
    ctx.write_rootfs_file("etc/NetworkManager/NetworkManager.conf", nm_conf);

    std::string netplan_conf =
        "network:\n"
        "  version: 2\n"
        "  renderer: NetworkManager\n";
    ctx.write_rootfs_file("etc/netplan/01-network-manager-all.yaml", netplan_conf);

    LOG_OK("NetworkManager and Netplan configured.");
    return true;
}

// -----------------------------------------------------------------------------
// 84 - Clean APT Cache
// -----------------------------------------------------------------------------
bool AptCacheCleanerStep::execute(StepContext& ctx) {
    LOG_INFO("Cleaning APT archives and system logs...");
    ctx.run_chroot("apt-get", {"clean"});
    FileOps::remove_all_safe(ctx.rootfs_path() / "var/cache/apt/archives");
    FileOps::ensure_directory_exists(ctx.rootfs_path() / "var/cache/apt/archives/partial");
    FileOps::remove_all_safe(ctx.rootfs_path() / "var/log");
    FileOps::ensure_directory_exists(ctx.rootfs_path() / "var/log");
    LOG_OK("APT cache and logs cleaned.");
    return true;
}

// -----------------------------------------------------------------------------
// 85 - Machine ID Wiper
// -----------------------------------------------------------------------------
bool MachineIdWiperStep::execute(StepContext& ctx) {
    LOG_INFO("Wiping machine-id to 0 bytes for fresh instance uniqueness...");
    FileOps::write_file_atomic(ctx.rootfs_path() / "etc/machine-id", "");
    FileOps::write_file_atomic(ctx.rootfs_path() / "var/lib/dbus/machine-id", "");
    LOG_OK("Machine-id wiped.");
    return true;
}

// -----------------------------------------------------------------------------
// 86 - Remove Diversions
// -----------------------------------------------------------------------------
bool DiversionRemoverStep::execute(StepContext& ctx) {
    LOG_INFO("Removing /sbin/initctl diversion...");
    ctx.run_chroot_shell("if [ -e /sbin/initctl ]; then rm -f /sbin/initctl; fi");
    ctx.run_chroot("dpkg-divert", {"--rename", "--remove", "/sbin/initctl"});
    LOG_OK("Initctl diversion removed.");
    return true;
}

// -----------------------------------------------------------------------------
// 87 - Clean History & Temp
// -----------------------------------------------------------------------------
bool HistoryCleanerStep::execute(StepContext& ctx) {
    LOG_INFO("Wiping temporary folders and shell history...");
    FileOps::remove_all_safe(ctx.rootfs_path() / "tmp");
    FileOps::ensure_directory_exists(ctx.rootfs_path() / "tmp");
    FileOps::remove_all_safe(ctx.rootfs_path() / "root/.bash_history");
    LOG_OK("History and temporary files cleaned.");
    return true;
}

// -----------------------------------------------------------------------------
// 88 - Clean Merged-Usr Residues
// -----------------------------------------------------------------------------
bool UselessFoldersCleanerStep::execute(StepContext& ctx) {
    LOG_INFO("Removing residual migration folders...");
    FileOps::remove_all_safe(ctx.rootfs_path() / "bin.usr-is-merged");
    FileOps::remove_all_safe(ctx.rootfs_path() / "lib.usr-is-merged");
    FileOps::remove_all_safe(ctx.rootfs_path() / "sbin.usr-is-merged");
    LOG_OK("Residual folders removed.");
    return true;
}

} // namespace vaxp
