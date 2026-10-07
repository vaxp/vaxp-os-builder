#include "steps/BuildSteps.hpp"
#include "core/Logger.hpp"
#include "fs/FileOps.hpp"

namespace vaxp {

// -----------------------------------------------------------------------------
// 21 - Ubiquity Installer
// -----------------------------------------------------------------------------
bool UbiquityStep::execute(StepContext& ctx) {
    LOG_INFO("Installing Ubiquity OS Installer...");

    std::vector<std::string> ubi_pkgs = {
        "install", "-y", "--no-install-recommends",
        "cryptsetup-initramfs", "secureboot-db", "btrfs-progs", "lvm2",
        "libfile-mimeinfo-perl", "libnet-dbus-perl", "libx11-protocol-perl", "x11-utils",
        "ubiquity", "ubiquity-casper", "ubiquity-frontend-gtk",
        "ubiquity-slideshow-ubuntu", "ubiquity-ubuntu-artwork"
    };

    auto res = ctx.run_chroot("apt-get", ubi_pkgs);
    if (!res.success()) {
        LOG_ERROR("Failed to install Ubiquity packages.");
        return false;
    }

    LOG_OK("Ubiquity installer packages installed.");
    return true;
}

// -----------------------------------------------------------------------------
// 22 - Ubiquity Slides & Launcher Patch
// -----------------------------------------------------------------------------
bool UbiquityPatchStep::execute(StepContext& ctx) {
    LOG_INFO("Deploying VAXP Ubiquity slideshow and patching launcher...");

    fs::path slides_src = ctx.assets_path() / "ubiquity-slides";
    fs::path slides_dst = ctx.rootfs_path() / "usr/share/ubiquity-slideshow/slides";

    if (fs::exists(slides_src)) {
        FileOps::ensure_directory_exists(slides_dst);
        FileOps::copy_directory_recursive(slides_src, slides_dst);
    }

    fs::path desktop_entry = ctx.rootfs_path() / "usr/share/applications/ubiquity.desktop";
    if (fs::exists(desktop_entry)) {
        std::string content = FileOps::read_file(desktop_entry);
        std::string search = "Exec=sudo --preserve-env=DBUS_SESSION_BUS_ADDRESS,XDG_DATA_DIRS,XDG_RUNTIME_DIR,GTK_THEME sh -c 'ubiquity gtk_ui'";
        std::string replacement = "Exec=sudo --preserve-env=DBUS_SESSION_BUS_ADDRESS,XDG_DATA_DIRS,XDG_RUNTIME_DIR,GTK_THEME,HOME sh -c 'ubiquity gtk_ui'";
        size_t pos = content.find(search);
        if (pos != std::string::npos) {
            content.replace(pos, search.length(), replacement);
            FileOps::write_file_atomic(desktop_entry, content);
        }
    }

    LOG_OK("Ubiquity slideshow and launcher patched.");
    return true;
}

// -----------------------------------------------------------------------------
// 23 - Software Properties GTK
// -----------------------------------------------------------------------------
bool SoftwarePropertiesGtkStep::execute(StepContext& ctx) {
    LOG_INFO("Patching software-properties-gtk to remove Ubuntu Pro...");

    ctx.run_chroot("apt-get", {"install", "-y", "--no-install-recommends", 
                               "python3-dateutil", "gir1.2-handy-1", "libgtk3-perl"});

    // Download and modify deb package in chroot /tmp
    std::string script = 
        "cd /tmp && "
        "rm -rf sp_mod && mkdir -p sp_mod && cd sp_mod && "
        "apt-get download software-properties-gtk && "
        "DEB=$(ls *.deb | head -n 1) && "
        "dpkg-deb -R \"$DEB\" original && "
        "sed -i '/^Depends:/s/, *ubuntu-pro-client//; /^Depends:/s/, *ubuntu-advantage-desktop-daemon//' original/DEBIAN/control && "
        "dpkg-deb -b original modified.deb && "
        "dpkg -i modified.deb || apt-get install -fy && "
        "cd /tmp && rm -rf sp_mod";

    ctx.run_chroot_shell(script);

    // Patch Python file
    fs::path py_file = ctx.rootfs_path() / "usr/lib/python3/dist-packages/softwareproperties/gtk/SoftwarePropertiesGtk.py";
    if (fs::exists(py_file)) {
        std::string content = FileOps::read_file(py_file);
        // Remove UbuntuProPage imports
        size_t pos = content.find("from .UbuntuProPage import UbuntuProPage");
        if (pos != std::string::npos) {
            content.erase(pos, content.find('\n', pos) - pos + 1);
            FileOps::write_file_atomic(py_file, content);
        }
    }

    // Pin software-properties-gtk
    ctx.run_chroot("apt-mark", {"hold", "software-properties-gtk"});
    std::string pin_pref =
        "Package: software-properties-gtk\n"
        "Pin: release o=Ubuntu\n"
        "Pin-Priority: -1\n";
    ctx.write_rootfs_file("etc/apt/preferences.d/no-upgrade-software-properties-gtk", pin_pref);

    LOG_OK("Software properties GTK patched and pinned.");
    return true;
}

// -----------------------------------------------------------------------------
// 23b - Wallpapers
// -----------------------------------------------------------------------------
bool WallpaperStep::execute(StepContext& ctx) {
    LOG_INFO("Deploying VAXP Desktop Wallpapers...");

    fs::path wp_src = ctx.assets_path() / "wallpapers";
    fs::path wp_dst = ctx.rootfs_path() / "usr/share/backgrounds";

    if (fs::exists(wp_src)) {
        FileOps::ensure_directory_exists(wp_dst);
        FileOps::copy_directory_recursive(wp_src, wp_dst);
    }

    LOG_OK("Wallpapers deployed.");
    return true;
}

// -----------------------------------------------------------------------------
// 24 - Fluent & Nordzy Icon Theme
// -----------------------------------------------------------------------------
bool FluentIconThemeStep::execute(StepContext& ctx) {
    LOG_INFO("Installing Nordzy icon theme and Sunity cursor pack...");

    fs::path themes_dir = ctx.assets_path() / "themes";
    fs::path nordzy_dir = themes_dir / "Nordzy-icon";
    fs::path sunity_dir = themes_dir / "sunity-cursors";

    if (fs::exists(nordzy_dir)) {
        ctx.copy_to_rootfs(nordzy_dir, "tmp/nordzy");
        ctx.run_chroot_shell("cd /tmp/nordzy && chmod +x install.sh && ./install.sh -t default -c -p 2>/dev/null || true");
        FileOps::remove_all_safe(ctx.rootfs_path() / "tmp/nordzy");
    }

    if (fs::exists(sunity_dir)) {
        ctx.copy_to_rootfs(sunity_dir, "tmp/sunity");
        ctx.run_chroot_shell("cd /tmp/sunity && chmod +x install.sh && ./install.sh 2>/dev/null || true");
        FileOps::remove_all_safe(ctx.rootfs_path() / "tmp/sunity");
    }

    LOG_OK("Icon and cursor themes installed.");
    return true;
}

// -----------------------------------------------------------------------------
// 25 - WhiteSur GTK Theme
// -----------------------------------------------------------------------------
bool FluentGtkThemeStep::execute(StepContext& ctx) {
    LOG_INFO("Deploying WhiteSur-Dark GTK theme...");

    fs::path theme_src = ctx.assets_path() / "themes/WhiteSur-Dark";
    fs::path theme_dst = ctx.rootfs_path() / "usr/share/themes/WhiteSur-Dark";

    if (fs::exists(theme_src)) {
        FileOps::ensure_directory_exists(theme_dst);
        FileOps::copy_directory_recursive(theme_src, theme_dst);
    }

    LOG_OK("WhiteSur-Dark theme deployed.");
    return true;
}

// -----------------------------------------------------------------------------
// 34 - Input Method
// -----------------------------------------------------------------------------
bool InputMethodStep::execute(StepContext& ctx) {
    LOG_INFO("Configuring language-selector input method dependencies...");

    fs::path patch_file = ctx.assets_path() / "input-method/pkg_depends_patch";
    fs::path target_pkg_depends = ctx.rootfs_path() / "usr/share/language-selector/data/pkg_depends";

    if (fs::exists(target_pkg_depends) && fs::exists(patch_file)) {
        std::string current = FileOps::read_file(target_pkg_depends);
        std::string filtered;
        std::stringstream ss(current);
        std::string line;
        while (std::getline(ss, line)) {
            if (line.rfind("im:", 0) != 0) {
                filtered += line + "\n";
            }
        }
        std::string patch_content = FileOps::read_file(patch_file);
        filtered += patch_content + "\n";
        FileOps::write_file_atomic(target_pkg_depends, filtered);
    }

    LOG_OK("Language selector patched.");
    return true;
}

// -----------------------------------------------------------------------------
// 36 - Ubuntu Logo Text
// -----------------------------------------------------------------------------
bool UbuntuLogoTextStep::execute(StepContext& ctx) {
    LOG_INFO("Replacing Ubuntu pixmaps with VAXP branding...");

    fs::path pix_src = ctx.assets_path() / "pixmaps";
    fs::path logo_light = pix_src / "ubuntu-logo-text.png";
    fs::path logo_dark  = pix_src / "ubuntu-logo-text-dark.png";

    if (fs::exists(logo_light)) {
        ctx.copy_to_rootfs(logo_light, "usr/share/pixmaps/ubuntu-logo-text.png");
    }
    if (fs::exists(logo_dark)) {
        ctx.copy_to_rootfs(logo_dark, "usr/share/pixmaps/ubuntu-logo-text-dark.png");
    }

    LOG_OK("Pixmaps updated.");
    return true;
}

// -----------------------------------------------------------------------------
// 37 - XDG MIME Applications
// -----------------------------------------------------------------------------
bool XdgMimeStep::execute(StepContext& ctx) {
    LOG_INFO("Configuring default VAXP file handlers...");

    std::string mime_content =
        "[Default Applications]\n"
        "inode/directory=aetherfiles.desktop\n"
        "text/plain=vcodex.desktop\n"
        "application/pdf=vpreview.desktop\n"
        "application/epub+zip=vpreview.desktop\n"
        "application/zip=varchive.desktop\n"
        "application/x-7z-compressed=varchive.desktop\n"
        "application/x-rar=varchive.desktop\n"
        "application/x-tar=varchive.desktop\n"
        "application/gzip=varchive.desktop\n"
        "application/x-bittorrent=vxap-downloader.desktop\n"
        "application/x-utorrent=vxap-downloader.desktop\n"
        "application/vnd.debian.binary-package=vaxpsam.desktop\n"
        "image/jpeg=vgallery.desktop\n"
        "image/png=vgallery.desktop\n"
        "image/bmp=vgallery.desktop\n"
        "image/webp=vgallery.desktop\n"
        "image/svg+xml=vgallery.desktop\n"
        "audio/mpeg=vaudio.desktop\n"
        "audio/mp3=vaudio.desktop\n"
        "audio/x-wav=vaudio.desktop\n"
        "audio/x-flac=vaudio.desktop\n"
        "audio/ogg=vaudio.desktop\n"
        "video/mp4=vvplayer.desktop\n"
        "video/x-matroska=vvplayer.desktop\n"
        "video/webm=vvplayer.desktop\n"
        "video/avi=vvplayer.desktop\n"
        "video/quicktime=vvplayer.desktop\n";

    ctx.write_rootfs_file("etc/skel/.config/mimeapps.list", mime_content);
    ctx.write_rootfs_file("usr/share/applications/defaults.list", mime_content);

    LOG_OK("MIME associations established.");
    return true;
}

// -----------------------------------------------------------------------------
// 38 - User Templates
// -----------------------------------------------------------------------------
bool TemplatesStep::execute(StepContext& ctx) {
    LOG_INFO("Generating document and programming templates in /etc/skel...");

    fs::path tpl_dir = ctx.rootfs_path() / "etc/skel/Templates";
    FileOps::ensure_directory_exists(tpl_dir);

    ctx.write_rootfs_file("etc/skel/Templates/Text.txt", "");
    ctx.write_rootfs_file("etc/skel/Templates/Markdown.md", "# Title\n\n- [ ] Task 1\n- [ ] Task 2\n");
    ctx.write_rootfs_file("etc/skel/Templates/Python.py", "def main():\n    print(\"Hello, Python!\")\n\nif __name__ == '__main__':\n    main()\n");
    ctx.write_rootfs_file("etc/skel/Templates/Dart.dart", "void main() {\n    print('Hello, Dart!');\n}\n");
    ctx.write_rootfs_file("etc/skel/Templates/C.c", "#include <stdio.h>\n\nint main() {\n    printf(\"Hello, C!\\n\");\n    return 0;\n}\n");
    ctx.write_rootfs_file("etc/skel/Templates/Cpp.cpp", "#include <iostream>\n\nint main() {\n    std::cout << \"Hello, C++!\\n\";\n    return 0;\n}\n");

    LOG_OK("User templates deployed.");
    return true;
}

// -----------------------------------------------------------------------------
// 39 - Aether & Desktop Configs
// -----------------------------------------------------------------------------
bool AetherConfigsStep::execute(StepContext& ctx) {
    LOG_INFO("Deploying Aether Shell and desktop environment configurations...");

    fs::path aether_cfg_src = ctx.assets_path() / "aether-configs";
    fs::path aether_cfg_dst = ctx.rootfs_path() / "etc/skel/.config";

    if (fs::exists(aether_cfg_src)) {
        FileOps::ensure_directory_exists(aether_cfg_dst);
        FileOps::copy_directory_recursive(aether_cfg_src, aether_cfg_dst);
    }

    LOG_OK("Aether Shell configurations deployed to /etc/skel/.config.");
    return true;
}

} // namespace vaxp
