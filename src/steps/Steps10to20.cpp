#include "steps/BuildSteps.hpp"
#include "core/Logger.hpp"
#include "fs/FileOps.hpp"
#include <unistd.h>

namespace vaxp {

// -----------------------------------------------------------------------------
// 10 - No Snap
// -----------------------------------------------------------------------------
bool NoSnapStep::execute(StepContext& ctx) {
    if (ctx.config().store_provider == StoreProvider::SNAP) {
        LOG_INFO("Snap store provider requested in configuration. Skipping snap purge.");
        return true;
    }

    LOG_INFO("Purging Snapd subsystem and applying APT pinning...");
    ctx.run_chroot_shell("snap remove snap-store 2>/dev/null || true");
    ctx.run_chroot_shell("snap remove gtk-common-themes 2>/dev/null || true");
    ctx.run_chroot_shell("snap remove snapd-desktop-integration 2>/dev/null || true");
    ctx.run_chroot_shell("snap remove bare 2>/dev/null || true");

    ctx.run_chroot("apt-get", {"purge", "-y", "snapd"});

    const std::vector<std::string> snap_dirs = {
        "snap", "var/snap", "var/lib/snapd", "var/cache/snapd", "usr/lib/snapd", "root/snap"
    };
    for (const auto& dir : snap_dirs) {
        FileOps::remove_all_safe(ctx.rootfs_path() / dir);
    }

    std::string no_snap_pref =
        "Package: snapd\n"
        "Pin: release a=*\n"
        "Pin-Priority: -10\n";
    ctx.write_rootfs_file("etc/apt/preferences.d/no-snap.pref", no_snap_pref);

    LOG_OK("Snapd purged and blocked.");
    return true;
}

// -----------------------------------------------------------------------------
// 12 - No MOTD
// -----------------------------------------------------------------------------
bool NoMotdStep::execute(StepContext& ctx) {
    LOG_INFO("Removing Ubuntu MOTD and update-manager notices...");
    FileOps::remove_all_safe(ctx.rootfs_path() / "etc/update-manager");
    FileOps::remove_all_safe(ctx.rootfs_path() / "etc/update-motd.d");
    LOG_OK("MOTD notices removed.");
    return true;
}

// -----------------------------------------------------------------------------
// 14 - VAXP Desktop Stack & Media Apps
// -----------------------------------------------------------------------------
bool VaxpAppsStep::execute(StepContext& ctx) {
    LOG_INFO("Installing desktop subsystems, PipeWire, and core applications...");

    std::vector<std::string> apps = {
        "install", "-y", "--no-install-recommends",
        "apt-transport-https", "cifs-utils", "cloud-init", "coreutils", "gnupg", "gpg",
        "gvfs-fuse", "gvfs-backends", "wsdd", "libsass1", "lsb-release",
        "systemd-timesyncd", "gdb", "sassc", "software-properties-common",
        "mesa-vulkan-drivers", "squashfs-tools", "sysstat", "wget", "whiptail",
        "gdisk", "eatmydata", "patch", "less", "gnupg-l10n", "gpg-wks-client",
        "upower", "mdadm", "appstream", "packagekit-tools", "python3-babel",
        "exfatprogs", "iw", "xxd", "xdg-utils", "zenity", "power-profiles-daemon",
        "xdg-desktop-portal", "xdg-desktop-portal-gtk", "xdg-desktop-portal-wlr",
        "bluez", "bluez-tools", "pulseaudio-utils", "dbus", "libdbus-1-dev",
        "rfkill", "brightnessctl",
        // Display Server
        "spice-vdagent", "xserver-xorg-input-all", "xserver-xorg", "xserver-xorg-legacy",
        "xserver-xorg-video-intel", "xserver-xorg-video-qxl", "xserver-xorg-video-all",
        "libcanberra-gtk3-0", "libcanberra-gtk3-module", "libcanberra-pulse",
        "libcanberra0", "libadwaita-1-0",
        // Plymouth
        "plymouth", "plymouth-label", "plymouth-theme-spinner", "plymouth-theme-ubuntu-text",
        // Network VPN
        "openvpn", "network-manager-openvpn", "network-manager-pptp",
        // Multimedia
        "gstreamer1.0-alsa", "gstreamer1.0-libav", "gstreamer1.0-gtk3", "gstreamer1.0-x",
        "gstreamer1.0-gl", "gstreamer1.0-tools", "gstreamer1.0-pipewire",
        "gstreamer1.0-packagekit", "gstreamer1.0-plugins-base-apps",
        "pipewire", "pipewire-audio-client-libraries", "wireplumber", "pipewire-pulse",
        "pipewire-alsa", "pipewire-jack", "libgtk-layer-shell0", "slurp", "grim",
        "libmpv2", "libpulse-dev",
        // IBus & Fonts
        "ibus", "ibus-gtk", "ibus-gtk3", "ibus-gtk4", "im-config",
        "fonts-noto-cjk", "fonts-noto-core", "fonts-noto-mono", "fonts-noto-color-emoji",
        // Python & Xorg
        "python3", "python3-pip", "python-is-python3", "pipx", "xorg"
    };

    auto res = ctx.run_chroot("apt-get", apps);
    if (!res.success()) {
        LOG_ERROR("Failed to install VAXP desktop stack packages.");
        return false;
    }

    // Enable PipeWire user services in /etc/skel
    fs::path user_systemd = ctx.rootfs_path() / "etc/skel/.config/systemd/user/default.target.wants";
    FileOps::ensure_directory_exists(user_systemd);
    ctx.run_chroot_shell(
        "ln -sf /usr/lib/systemd/user/pipewire.service /etc/skel/.config/systemd/user/default.target.wants/pipewire.service && "
        "ln -sf /usr/lib/systemd/user/pipewire-pulse.service /etc/skel/.config/systemd/user/default.target.wants/pipewire-pulse.service && "
        "ln -sf /usr/lib/systemd/user/wireplumber.service /etc/skel/.config/systemd/user/default.target.wants/wireplumber.service"
    );

    // Remove legacy terminal desktop shortcuts
    FileOps::remove_all_safe(ctx.rootfs_path() / "usr/share/applications/htop.desktop");
    FileOps::remove_all_safe(ctx.rootfs_path() / "usr/share/applications/vim.desktop");

    LOG_OK("Desktop stack and user services installed.");
    return true;
}

// -----------------------------------------------------------------------------
// 15 - Fonts
// -----------------------------------------------------------------------------
bool FontsStep::execute(StepContext& ctx) {
    LOG_INFO("Deploying typography configuration and font bundles...");

    fs::path fonts_dir = ctx.assets_path() / "fonts";
    fs::path local_conf = fonts_dir / "local.conf";
    fs::path fonts_zip  = fonts_dir / "fonts.zip";

    if (fs::exists(local_conf)) {
        ctx.copy_to_rootfs(local_conf, "etc/fonts/local.conf");
    }

    if (fs::exists(fonts_zip)) {
        LOG_INFO("Extracting font bundle: " + fonts_zip.string());
        ctx.copy_to_rootfs(fonts_zip, "tmp/fonts.zip");
        ctx.run_chroot_shell("unzip -q -O UTF-8 /tmp/fonts.zip -d /usr/share/fonts/ && rm /tmp/fonts.zip");
    }

    LOG_INFO("Rebuilding font cache...");
    ctx.run_chroot("fc-cache", {"-f"});

    LOG_OK("Typography setup completed.");
    return true;
}

// -----------------------------------------------------------------------------
// 17 - App Store Launcher
// -----------------------------------------------------------------------------
bool AppStoreStep::execute(StepContext& ctx) {
    LOG_INFO("Generating VAXP Application Store desktop entry...");

    std::string desktop_entry =
        "[Desktop Entry]\n"
        "Name=Apps Store\n"
        "GenericName=Apps Store\n"
        "Name[zh_CN]=应用商店\n"
        "Name[zh_TW]=應用商店\n"
        "Name[zh_HK]=應用商店\n"
        "Name[ja_JP]=アプリストア\n"
        "Name[ko_KR]=앱 스토어\n"
        "Name[vi_VN]=Cửa hàng ứng dụng\n"
        "Name[th_TH]=ร้านค้าแอปพลิเคชัน\n"
        "Name[de_DE]=App-Store\n"
        "Name[fr_FR]=Magasin d'applications\n"
        "Name[es_ES]=Tienda de aplicaciones\n"
        "Name[ru_RU]=Магазин приложений\n"
        "Name[it_IT]=Negozio di applicazioni\n"
        "Name[pt_PT]=Loja de aplicativos\n"
        "Name[pt_BR]=Loja de aplicativos\n"
        "Name[ar_SA]=متجر التطبيقات\n"
        "Name[nl_NL]=App Store\n"
        "Name[sv_SE]=App Store\n"
        "Name[pl_PL]=Sklep z aplikacjami\n"
        "Name[tr_TR]=Uygulama Mağazası\n"
        "Comment=Browse VAXP-OS's software collection and install our verified applications\n"
        "Comment[ar_SA]=تصفح مجموعة البرامج الخاصة بـ VAXP-OS وقم بتثبيت تطبيقاتنا الموثقة\n"
        "Categories=System;\n"
        "Exec=xdg-open https://docs.vaxp.org/\n"
        "Terminal=false\n"
        "Type=Application\n"
        "Icon=system-software-install\n"
        "StartupNotify=true\n";

    ctx.write_rootfs_file("usr/share/applications/VAXP-OS-software.desktop", desktop_entry);
    LOG_OK("Application Store desktop entry created.");
    return true;
}

// -----------------------------------------------------------------------------
// 18 - Custom VAXP Deb Packages & Aether
// -----------------------------------------------------------------------------
bool AvaxpInstallAppsStep::execute(StepContext& ctx) {
    LOG_INFO("Installing custom VAXP deb packages (Aether Shell & V-Apps)...");

    fs::path deb_src_dir = ctx.assets_path() / "vaxpdeb";
    std::error_code ec;

    if (!fs::exists(deb_src_dir, ec)) {
        LOG_WARN("Custom deb packages directory not found at: " + deb_src_dir.string());
        return true;
    }

    fs::path target_tmp_dir = ctx.rootfs_path() / "tmp/vaxpdeb";
    FileOps::remove_all_safe(target_tmp_dir);
    FileOps::ensure_directory_exists(target_tmp_dir);

    FileOps::copy_directory_recursive(deb_src_dir, target_tmp_dir);

    LOG_INFO("Executing apt-get batch install for custom packages...");
    auto inst_res = ctx.run_chroot_shell("apt-get install -y /tmp/vaxpdeb/*.deb");
    if (!inst_res.success()) {
        LOG_WARN("Resolving unmet dependencies via apt-get install -f -y...");
        auto fix_res = ctx.run_chroot("apt-get", {"install", "-f", "-y"});
        if (!fix_res.success()) {
            LOG_ERROR("Failed to resolve dependencies for custom deb packages.");
            FileOps::remove_all_safe(target_tmp_dir);
            return false;
        }
    }

    FileOps::remove_all_safe(target_tmp_dir);
    LOG_OK("Custom VAXP applications and Aether Shell components installed.");
    return true;
}

// -----------------------------------------------------------------------------
// 19 - Plymouth Patch
// -----------------------------------------------------------------------------
bool PlymouthPatchStep::execute(StepContext& ctx) {
    LOG_INFO("Deploying Plymouth boot animation branding...");

    fs::path plymouth_src = ctx.assets_path() / "plymouth";
    fs::path logo_128 = plymouth_src / "logo_128.png";
    fs::path text_png = plymouth_src / "vaxp-os_text.png";

    if (fs::exists(logo_128)) {
        FileOps::ensure_directory_exists(ctx.rootfs_path() / "usr/share/plymouth/themes/spinner");
        ctx.copy_to_rootfs(logo_128, "usr/share/plymouth/themes/spinner/bgrt-fallback.png");
    }
    if (fs::exists(text_png)) {
        ctx.copy_to_rootfs(text_png, "usr/share/plymouth/ubuntu-logo.png");
        ctx.copy_to_rootfs(text_png, "usr/share/plymouth/themes/spinner/watermark.png");
    }

    LOG_OK("Plymouth branding applied.");
    return true;
}

// -----------------------------------------------------------------------------
// 20 - Native Deskmon Daemon (Zero GLib)
// -----------------------------------------------------------------------------
bool DeskmonStep::execute(StepContext& ctx) {
    LOG_INFO("Configuring Native Deskmon Daemon (C++20, inotify, 0% GLib)...");

    std::string deskmon_source = R"(#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <unistd.h>
#include <signal.h>
#include <cstring>

namespace fs = std::filesystem;
static volatile sig_atomic_t g_running = 1;

static void signal_handler(int) { g_running = 0; }

static void trust_desktop_file(const fs::path& file_path) {
    std::error_code ec;
    if (!fs::exists(file_path, ec)) return;

    fs::permissions(file_path,
                    fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec,
                    fs::perm_options::add, ec);

    const char val[] = "true";
    setxattr(file_path.c_str(), "user.metadata::trusted", val, sizeof(val), 0);
    setxattr(file_path.c_str(), "metadata::trusted", val, sizeof(val), 0);
}

static void scan_initial(const fs::path& desktop_dir) {
    std::error_code ec;
    if (!fs::exists(desktop_dir, ec)) return;

    for (const auto& entry : fs::directory_iterator(desktop_dir, ec)) {
        if (entry.is_regular_file(ec) && entry.path().extension() == ".desktop") {
            trust_desktop_file(entry.path());
        }
    }
}

int main() {
    const char* home = getenv("HOME");
    if (!home) return 1;

    fs::path desktop_dir = fs::path(home) / "Desktop";
    std::error_code ec;
    fs::create_directories(desktop_dir, ec);

    struct sigaction sa{};
    sa.sa_handler = signal_handler;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);

    scan_initial(desktop_dir);

    int inotify_fd = inotify_init1(IN_NONBLOCK);
    if (inotify_fd < 0) return 1;

    int wd = inotify_add_watch(inotify_fd, desktop_dir.c_str(), IN_CREATE | IN_MOVED_TO);
    if (wd < 0) {
        close(inotify_fd);
        return 1;
    }

    constexpr size_t BUF_LEN = 1024 * (sizeof(struct inotify_event) + 256);
    char buffer[BUF_LEN];

    while (g_running) {
        ssize_t len = read(inotify_fd, buffer, sizeof(buffer));
        if (len < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(250000);
                continue;
            }
            break;
        }

        size_t i = 0;
        while (i < static_cast<size_t>(len)) {
            auto* event = reinterpret_cast<struct inotify_event*>(&buffer[i]);
            if (event->len > 0) {
                std::string filename = event->name;
                if (filename.length() > 8 && filename.substr(filename.length() - 8) == ".desktop") {
                    trust_desktop_file(desktop_dir / filename);
                }
            }
            i += sizeof(struct inotify_event) + event->len;
        }
    }

    inotify_rm_watch(inotify_fd, wd);
    close(inotify_fd);
    return 0;
}
)";

    fs::path temp_cpp = ctx.workspace_root() / "VAXP-OS-Builder/deskmon_native.cpp";
    FileOps::write_file_atomic(temp_cpp, deskmon_source);

    fs::path target_bin = ctx.rootfs_path() / "usr/local/bin/deskmon";
    FileOps::ensure_directory_exists(target_bin.parent_path());

    LOG_INFO("Compiling pure native deskmon binary...");
    auto comp_res = ctx.run_host("g++", {
        "-std=c++20", "-O2", "-s",
        temp_cpp.string(),
        "-o", target_bin.string()
    });

    std::error_code ec;
    fs::remove(temp_cpp, ec);

    if (!comp_res.success()) {
        LOG_ERROR("Failed to compile native deskmon.");
        return false;
    }

    fs::permissions(target_bin, 
                    fs::perms::owner_read | fs::perms::owner_write | fs::perms::owner_exec |
                    fs::perms::group_read | fs::perms::group_exec |
                    fs::perms::others_read | fs::perms::others_exec,
                    fs::perm_options::replace, ec);

    std::string service_file = 
        "[Unit]\n"
        "Description=VAXP-OS Desktop Launcher Supervisor\n"
        "After=default.target\n\n"
        "[Service]\n"
        "Type=simple\n"
        "ExecStart=/usr/local/bin/deskmon\n"
        "Restart=on-failure\n"
        "RestartSec=3\n\n"
        "[Install]\n"
        "WantedBy=default.target\n";

    ctx.write_rootfs_file("etc/systemd/user/deskmon.service", service_file);

    fs::path skel_systemd = ctx.rootfs_path() / "etc/skel/.config/systemd/user/default.target.wants";
    FileOps::ensure_directory_exists(skel_systemd);
    ctx.run_chroot_shell("ln -sf /etc/systemd/user/deskmon.service /etc/skel/.config/systemd/user/default.target.wants/deskmon.service");

    LOG_OK("Deskmon daemon installed and enabled.");
    return true;
}

// -----------------------------------------------------------------------------
// 20b - No Sudo Hint
// -----------------------------------------------------------------------------
bool NoSudoHintStep::execute(StepContext& ctx) {
    LOG_INFO("Disabling sudo hint in /etc/bash.bashrc...");

    fs::path bashrc = ctx.rootfs_path() / "etc/bash.bashrc";
    if (fs::exists(bashrc)) {
        std::string content = FileOps::read_file(bashrc);
        size_t pos = content.find("# sudo hint");
        if (pos != std::string::npos) {
            size_t end_fi = content.find("fi", pos);
            if (end_fi != std::string::npos) {
                // Comment out the block
                std::string block = content.substr(pos, end_fi - pos + 2);
                std::string commented;
                std::stringstream ss(block);
                std::string line;
                while (std::getline(ss, line)) {
                    commented += "# " + line + "\n";
                }
                content.replace(pos, block.length(), commented);
                FileOps::write_file_atomic(bashrc, content);
            }
        }
    }

    LOG_OK("Sudo hint disabled.");
    return true;
}

} // namespace vaxp
