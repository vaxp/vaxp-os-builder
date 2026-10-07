#pragma once

#include "engine/IBuildStep.hpp"

namespace vaxp {

// 00 - Host & Environment
class CheckHostStep : public IBuildStep {
public:
    std::string id() const override { return "00-check-host-mod"; }
    std::string title() const override { return "Validate Host Environment & Dependencies"; }
    std::string description() const override { return "Checks root privileges, host tools, and available disk space"; }
    bool requires_chroot_mounts() const override { return false; }
    bool execute(StepContext& ctx) override;
};

// 01 - APT Sources
class AptSourceStep : public IBuildStep {
public:
    std::string id() const override { return "01-apt-source-mod"; }
    std::string title() const override { return "Configure Build APT Repositories"; }
    std::string description() const override { return "Generates /etc/apt/sources.list with build mirrors"; }
    bool requires_chroot_mounts() const override { return false; }
    bool execute(StepContext& ctx) override;
};

// 02 - Hostname & Base Locales
class SetHostnameStep : public IBuildStep {
public:
    std::string id() const override { return "02-set-hostname-mod"; }
    std::string title() const override { return "Set Hostname and Base Locales"; }
    std::string description() const override { return "Sets system hostname and generates initial locales"; }
    bool requires_chroot_mounts() const override { return true; }
    bool execute(StepContext& ctx) override;
};

// 03 - Systemd Subsystem
class SystemdStep : public IBuildStep {
public:
    std::string id() const override { return "03-systemd-mod"; }
    std::string title() const override { return "Install Systemd & Core Services"; }
    std::string description() const override { return "Installs systemd-sysv, resolvconf, and core daemons"; }
    bool requires_chroot_mounts() const override { return true; }
    bool execute(StepContext& ctx) override;
};

// 04 - Machine ID Setup
class MachineIdStep : public IBuildStep {
public:
    std::string id() const override { return "04-machine-id-mod"; }
    std::string title() const override { return "Initialize Machine ID"; }
    std::string description() const override { return "Generates temporary machine ID and DBus links"; }
    bool requires_chroot_mounts() const override { return false; }
    bool execute(StepContext& ctx) override;
};

// 05 - Initctl Diversion
class InitctlStep : public IBuildStep {
public:
    std::string id() const override { return "05-initctl-mod"; }
    std::string title() const override { return "Setup Initctl Diversion"; }
    std::string description() const override { return "Diverts /sbin/initctl to prevent daemon starting during build"; }
    bool requires_chroot_mounts() const override { return true; }
    bool execute(StepContext& ctx) override;
};

// 06 - APT Upgrade
class AptUpgradeStep : public IBuildStep {
public:
    std::string id() const override { return "06-apt-upgrade-mod"; }
    std::string title() const override { return "Upgrade Base Packages"; }
    std::string description() const override { return "Runs apt-get upgrade on the bootstrapped base packages"; }
    bool requires_chroot_mounts() const override { return true; }
    bool execute(StepContext& ctx) override;
};

// 07 - System Tools & Drivers
class SystemToolsInstallStep : public IBuildStep {
public:
    std::string id() const override { return "07-system-tools-install-mod"; }
    std::string title() const override { return "Install System Tools & Graphics Drivers"; }
    std::string description() const override { return "Installs hardware tools, Mesa Vulkan/VA-API drivers, and pins base-files"; }
    bool requires_chroot_mounts() const override { return true; }
    bool execute(StepContext& ctx) override;
};

// 08 - Casper & Linux Kernel
class CasperKernelInstallStep : public IBuildStep {
public:
    std::string id() const override { return "08-casper-and-kernel-install-mod"; }
    std::string title() const override { return "Install Casper & Linux HWE Kernel"; }
    std::string description() const override { return "Installs Casper live boot environment, thermald, and latest kernel"; }
    bool requires_chroot_mounts() const override { return true; }
    bool execute(StepContext& ctx) override;
};

// 10 - Purge Snapd
class NoSnapStep : public IBuildStep {
public:
    std::string id() const override { return "10-no-snap-mod"; }
    std::string title() const override { return "Purge and Block Snapd Subsystem"; }
    std::string description() const override { return "Removes Snap packages and applies APT pinning against snapd"; }
    bool requires_chroot_mounts() const override { return true; }
    bool execute(StepContext& ctx) override;
};

// 12 - No MOTD
class NoMotdStep : public IBuildStep {
public:
    std::string id() const override { return "12-no-motd-mod"; }
    std::string title() const override { return "Remove Ubuntu MOTD & Update Manager Notifications"; }
    std::string description() const override { return "Deletes /etc/update-motd.d and /etc/update-manager"; }
    bool requires_chroot_mounts() const override { return false; }
    bool execute(StepContext& ctx) override;
};

// 14 - VAXP Desktop & Media Apps
class VaxpAppsStep : public IBuildStep {
public:
    std::string id() const override { return "14-vaxp-apps-mod"; }
    std::string title() const override { return "Install Desktop Stack, PipeWire & Subsystems"; }
    std::string description() const override { return "Installs PipeWire, WirePlumber, GStreamer, Plymouth, and user daemons"; }
    bool requires_chroot_mounts() const override { return true; }
    bool execute(StepContext& ctx) override;
};

// 15 - Fonts
class FontsStep : public IBuildStep {
public:
    std::string id() const override { return "15-fonts-mod"; }
    std::string title() const override { return "Deploy Typography & Font Cache"; }
    std::string description() const override { return "Deploys Noto CJK, Cascadia Code, local.conf, and refreshes fc-cache"; }
    bool requires_chroot_mounts() const override { return true; }
    bool execute(StepContext& ctx) override;
};

// 17 - App Store Launcher
class AppStoreStep : public IBuildStep {
public:
    std::string id() const override { return "17-appstore-app"; }
    std::string title() const override { return "Configure VAXP Application Store Entry"; }
    std::string description() const override { return "Generates multilingual VAXP-OS-software.desktop launcher"; }
    bool requires_chroot_mounts() const override { return false; }
    bool execute(StepContext& ctx) override;
};

// 18 - Custom VAXP Deb Packages & Aether
class AvaxpInstallAppsStep : public IBuildStep {
public:
    std::string id() const override { return "18-avaxp-install-apps-mod"; }
    std::string title() const override { return "Install Custom VAXP Applications & Aether"; }
    std::string description() const override { return "Installs 50+ VAXP-OS deb packages with dependency resolution"; }
    bool requires_chroot_mounts() const override { return true; }
    bool execute(StepContext& ctx) override;
};

// 19 - Plymouth Patch
class PlymouthPatchStep : public IBuildStep {
public:
    std::string id() const override { return "19-plymouth-patch"; }
    std::string title() const override { return "Patch Plymouth Boot Branding"; }
    std::string description() const override { return "Applies VAXP-OS logos and spinner watermark"; }
    bool requires_chroot_mounts() const override { return false; }
    bool execute(StepContext& ctx) override;
};

// 20 - Native Deskmon Daemon
class DeskmonStep : public IBuildStep {
public:
    std::string id() const override { return "20-deskmon-mod"; }
    std::string title() const override { return "Build & Install Native Deskmon Daemon"; }
    std::string description() const override { return "Pure C++ inotify desktop supervisor daemon (0% GLib) and systemd service"; }
    bool requires_chroot_mounts() const override { return false; }
    bool execute(StepContext& ctx) override;
};

// 20b - No Sudo Hint
class NoSudoHintStep : public IBuildStep {
public:
    std::string id() const override { return "20-no-sudo-hint-mod"; }
    std::string title() const override { return "Remove Sudo Hint from Bash"; }
    std::string description() const override { return "Removes the default Ubuntu sudo hint message in /etc/bash.bashrc"; }
    bool requires_chroot_mounts() const override { return false; }
    bool execute(StepContext& ctx) override;
};

// 21 - Ubiquity Installer
class UbiquityStep : public IBuildStep {
public:
    std::string id() const override { return "21-ubiquity-mod"; }
    std::string title() const override { return "Install Ubiquity System Installer"; }
    std::string description() const override { return "Installs Ubiquity, ubiquity-casper, and partition management tools"; }
    bool requires_chroot_mounts() const override { return true; }
    bool execute(StepContext& ctx) override;
};

// 22 - Ubiquity Slides & Exec Patch
class UbiquityPatchStep : public IBuildStep {
public:
    std::string id() const override { return "22-ubiquity-patch"; }
    std::string title() const override { return "Patch Ubiquity Slideshow & Launcher"; }
    std::string description() const override { return "Deploys custom VAXP installer slideshow and environment preserves"; }
    bool requires_chroot_mounts() const override { return false; }
    bool execute(StepContext& ctx) override;
};

// 23 - Software Properties GTK
class SoftwarePropertiesGtkStep : public IBuildStep {
public:
    std::string id() const override { return "23-software-properties-gtk"; }
    std::string title() const override { return "Patch Software Properties GTK"; }
    std::string description() const override { return "Removes Ubuntu Pro dependencies and pins software-properties-gtk"; }
    bool requires_chroot_mounts() const override { return true; }
    bool execute(StepContext& ctx) override;
};

// 23b - Wallpapers
class WallpaperStep : public IBuildStep {
public:
    std::string id() const override { return "23-wallpaper-mod"; }
    std::string title() const override { return "Deploy VAXP Desktop Wallpapers"; }
    std::string description() const override { return "Deploys custom high-resolution wallpapers to /usr/share/backgrounds"; }
    bool requires_chroot_mounts() const override { return false; }
    bool execute(StepContext& ctx) override;
};

// 24 - Fluent & Nordzy Icon Theme
class FluentIconThemeStep : public IBuildStep {
public:
    std::string id() const override { return "24-fluent-icon-theme"; }
    std::string title() const override { return "Install Nordzy Icons & Sunity Cursors"; }
    std::string description() const override { return "Installs Nordzy icon theme and Sunity cursor pack"; }
    bool requires_chroot_mounts() const override { return true; }
    bool execute(StepContext& ctx) override;
};

// 25 - WhiteSur GTK Theme
class FluentGtkThemeStep : public IBuildStep {
public:
    std::string id() const override { return "25-fluent-gtk-theme"; }
    std::string title() const override { return "Install WhiteSur-Dark GTK Theme"; }
    std::string description() const override { return "Deploys WhiteSur-Dark theme to /usr/share/themes"; }
    bool requires_chroot_mounts() const override { return false; }
    bool execute(StepContext& ctx) override;
};

// 34 - Input Method (IBus)
class InputMethodStep : public IBuildStep {
public:
    std::string id() const override { return "34-input-method-mod"; }
    std::string title() const override { return "Configure Input Method & Language Selector"; }
    std::string description() const override { return "Patches language-selector and configures IBus frameworks"; }
    bool requires_chroot_mounts() const override { return true; }
    bool execute(StepContext& ctx) override;
};

// 36 - Ubuntu Logo Text Replacement
class UbuntuLogoTextStep : public IBuildStep {
public:
    std::string id() const override { return "36-ubuntu-logo-text"; }
    std::string title() const override { return "Replace Ubuntu Logo Pixmaps"; }
    std::string description() const override { return "Replaces logo text pixmaps in /usr/share/pixmaps"; }
    bool requires_chroot_mounts() const override { return false; }
    bool execute(StepContext& ctx) override;
};

// 37 - XDG MIME Applications
class XdgMimeStep : public IBuildStep {
public:
    std::string id() const override { return "37-xdg-mime-mod"; }
    std::string title() const override { return "Configure Default VAXP MIME Associations"; }
    std::string description() const override { return "Associates file formats with VAXP native applications"; }
    bool requires_chroot_mounts() const override { return false; }
    bool execute(StepContext& ctx) override;
};

// 38 - User Templates
class TemplatesStep : public IBuildStep {
public:
    std::string id() const override { return "38-templates-mod"; }
    std::string title() const override { return "Generate User Document Templates"; }
    std::string description() const override { return "Creates templates for Python, C, C++, Dart, Markdown, Text in /etc/skel"; }
    bool requires_chroot_mounts() const override { return false; }
    bool execute(StepContext& ctx) override;
};

// 39 - Aether & Venom Configs
class AetherConfigsStep : public IBuildStep {
public:
    std::string id() const override { return "39-co-pk"; }
    std::string title() const override { return "Deploy Aether & VAXP Desktop Configurations"; }
    std::string description() const override { return "Deploys Aether, Venom, Theme configs to /etc/skel/.config"; }
    bool requires_chroot_mounts() const override { return false; }
    bool execute(StepContext& ctx) override;
};

// 40 - Custom Extension Packages
class CopPkgeStep : public IBuildStep {
public:
    std::string id() const override { return "40-cop-pkge"; }
    std::string title() const override { return "Deploy Package Extension Artifacts"; }
    std::string description() const override { return "Copies supplementary package files to staging directory"; }
    bool requires_chroot_mounts() const override { return false; }
    bool execute(StepContext& ctx) override;
};

// 40.9 - Interactive Manual Control Prompt
class ManualControlStep : public IBuildStep {
public:
    std::string id() const override { return "40.9-con-man"; }
    std::string title() const override { return "Manual Chroot Control Window"; }
    std::string description() const override { return "Optional 60-second interactive terminal session inside chroot"; }
    bool requires_chroot_mounts() const override { return true; }
    bool execute(StepContext& ctx) override;
};

// 41 - Target APT Live Mirror
class TargetAptMirrorStep : public IBuildStep {
public:
    std::string id() const override { return "41-target-apt-mirror-mod"; }
    std::string title() const override { return "Configure Live Target APT Repositories"; }
    std::string description() const override { return "Sets live system mirrors in /etc/apt/sources.list"; }
    bool requires_chroot_mounts() const override { return false; }
    bool execute(StepContext& ctx) override;
};

// 42 - Intel SOF Audio Firmware
class IntelTheSofStep : public IBuildStep {
public:
    std::string id() const override { return "42-intel-thesof-mod"; }
    std::string title() const override { return "Install Intel SOF & ALSA UCM Audio Drivers"; }
    std::string description() const override { return "Downloads and deploys Intel Sound Open Firmware and ALSA UCM2 conf"; }
    bool requires_chroot_mounts() const override { return true; }
    bool execute(StepContext& ctx) override;
};

// 42b - Sessions & AppArmor Security
class SessionsPatchStep : public IBuildStep {
public:
    std::string id() const override { return "42-sessions-patch"; }
    std::string title() const override { return "Configure AppArmor User Namespaces Policy"; }
    std::string description() const override { return "Sets kernel.apparmor_restrict_unprivileged_userns = 0"; }
    bool requires_chroot_mounts() const override { return false; }
    bool execute(StepContext& ctx) override;
};

// 43 - Branding & Release Info
class EtcBrandingStep : public IBuildStep {
public:
    std::string id() const override { return "43-etc-branding-mod"; }
    std::string title() const override { return "Deploy Official VAXP-OS Identity Files"; }
    std::string description() const override { return "Writes /etc/os-release and /etc/lsb-release"; }
    bool requires_chroot_mounts() const override { return false; }
    bool execute(StepContext& ctx) override;
};

// 44 - Casper Configuration
class CasperPatchStep : public IBuildStep {
public:
    std::string id() const override { return "44-casper-patch"; }
    std::string title() const override { return "Configure Casper Live Boot Session"; }
    std::string description() const override { return "Generates /etc/casper.conf with VAXP live flavour"; }
    bool requires_chroot_mounts() const override { return false; }
    bool execute(StepContext& ctx) override;
};

// 45 - Issue & Login Banners
class EtcIssuePatchStep : public IBuildStep {
public:
    std::string id() const override { return "45-etc-issue-patch"; }
    std::string title() const override { return "Configure TTY Login Banners"; }
    std::string description() const override { return "Writes /etc/issue and /etc/issue.net"; }
    bool requires_chroot_mounts() const override { return false; }
    bool execute(StepContext& ctx) override;
};

// 78 - Disable Advertisements
class NoAdvertisementsStep : public IBuildStep {
public:
    std::string id() const override { return "78-no-advertisements-mod"; }
    std::string title() const override { return "Verify Absence of Ubuntu Pro Advertisements"; }
    std::string description() const override { return "Ensures 20apt-esm-hook.conf is removed from APT config"; }
    bool requires_chroot_mounts() const override { return false; }
    bool execute(StepContext& ctx) override;
};

// 79 - Purge Bloatware
class UselessPackageRemoverStep : public IBuildStep {
public:
    std::string id() const override { return "79-useless-package-remover"; }
    std::string title() const override { return "Purge Unnecessary Bloatware Packages"; }
    std::string description() const override { return "Removes GNOME games, telemetry daemons, and unused dependencies"; }
    bool requires_chroot_mounts() const override { return true; }
    bool execute(StepContext& ctx) override;
};

// 80 - Update Initramfs
class InitramfsUpdateStep : public IBuildStep {
public:
    std::string id() const override { return "80-initramfs-update"; }
    std::string title() const override { return "Update Initramfs Boot Images"; }
    std::string description() const override { return "Runs update-initramfs -u -k all inside chroot"; }
    bool requires_chroot_mounts() const override { return true; }
    bool execute(StepContext& ctx) override;
};

// 82 - Final Locales & Timezone
class LocalesConfigStep : public IBuildStep {
public:
    std::string id() const override { return "82-locales-config"; }
    std::string title() const override { return "Configure Final Timezone and Locales"; }
    std::string description() const override { return "Sets /etc/timezone, /etc/localtime, and /etc/default/locale"; }
    bool requires_chroot_mounts() const override { return true; }
    bool execute(StepContext& ctx) override;
};

// 83 - NetworkManager Configuration
class NetworkManagerPatchStep : public IBuildStep {
public:
    std::string id() const override { return "83-network-manager-patch"; }
    std::string title() const override { return "Configure NetworkManager and Netplan"; }
    std::string description() const override { return "Writes /etc/NetworkManager/NetworkManager.conf and Netplan config"; }
    bool requires_chroot_mounts() const override { return true; }
    bool execute(StepContext& ctx) override;
};

// 84 - Clean APT Cache
class AptCacheCleanerStep : public IBuildStep {
public:
    std::string id() const override { return "84-apt-cache-cleaner"; }
    std::string title() const override { return "Clean APT Caches and Logs"; }
    std::string description() const override { return "Clears /var/cache/apt/archives and system log files"; }
    bool requires_chroot_mounts() const override { return true; }
    bool execute(StepContext& ctx) override;
};

// 85 - Machine ID Wiper
class MachineIdWiperStep : public IBuildStep {
public:
    std::string id() const override { return "85-machine-id-wiper"; }
    std::string title() const override { return "Wipe Machine ID for First Boot Uniqueness"; }
    std::string description() const override { return "Truncates /etc/machine-id and /var/lib/dbus/machine-id to 0 bytes"; }
    bool requires_chroot_mounts() const override { return false; }
    bool execute(StepContext& ctx) override;
};

// 86 - Remove Diversions
class DiversionRemoverStep : public IBuildStep {
public:
    std::string id() const override { return "86-diversion-remover"; }
    std::string title() const override { return "Remove Initctl Diversion"; }
    std::string description() const override { return "Removes /sbin/initctl diversion and restores original binary"; }
    bool requires_chroot_mounts() const override { return true; }
    bool execute(StepContext& ctx) override;
};

// 87 - Clean History & Temp
class HistoryCleanerStep : public IBuildStep {
public:
    std::string id() const override { return "87-history-cleaner"; }
    std::string title() const override { return "Clean Bash History and Temp Files"; }
    std::string description() const override { return "Deletes /tmp/* and ~/.bash_history"; }
    bool requires_chroot_mounts() const override { return false; }
    bool execute(StepContext& ctx) override;
};

// 88 - Clean Merged-Usr Residues
class UselessFoldersCleanerStep : public IBuildStep {
public:
    std::string id() const override { return "88-useless-folders-cleaner"; }
    std::string title() const override { return "Remove Usr-is-Merged Artifacts"; }
    std::string description() const override { return "Deletes legacy /bin.usr-is-merged and related artifacts"; }
    bool requires_chroot_mounts() const override { return false; }
    bool execute(StepContext& ctx) override;
};

} // namespace vaxp
