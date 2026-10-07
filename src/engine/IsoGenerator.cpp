#include "engine/IsoGenerator.hpp"
#include "fs/SquashFs.hpp"
#include "fs/FileOps.hpp"
#include "core/Logger.hpp"
#include "core/Process.hpp"

#include <chrono>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <regex>

namespace vaxp {

namespace {

std::string get_current_date_str() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    struct tm tm_buf{};
    gmtime_r(&in_time_t, &tm_buf);
    ss << std::put_time(&tm_buf, "%y%m%d%H%M");
    return ss.str();
}

} // anonymous namespace

bool IsoGenerator::prepare_image_layout(StepContext& ctx) {
    const auto& img = ctx.image_path();
    std::error_code ec;
    fs::remove_all(img, ec);

    if (!FileOps::ensure_directory_exists(img / "casper") ||
        !FileOps::ensure_directory_exists(img / "isolinux") ||
        !FileOps::ensure_directory_exists(img / ".disk")) {
        LOG_ERROR("Failed to create image directory structure.");
        return false;
    }
    return true;
}

bool IsoGenerator::copy_kernel_and_initrd(StepContext& ctx) {
    const auto& rootfs = ctx.rootfs_path();
    const auto& img    = ctx.image_path();
    fs::path boot_dir  = rootfs / "boot";

    LOG_INFO("Searching for kernel and initrd in: " + boot_dir.string());

    fs::path vmlinuz_src, initrd_src;
    std::error_code ec;

    if (!fs::exists(boot_dir, ec)) {
        LOG_ERROR("/boot directory missing in rootfs!");
        return false;
    }

    for (const auto& entry : fs::directory_iterator(boot_dir, ec)) {
        std::string filename = entry.path().filename().string();
        if (filename.rfind("vmlinuz-", 0) == 0 && filename.find("generic") != std::string::npos) {
            vmlinuz_src = entry.path();
        } else if (filename.rfind("initrd.img-", 0) == 0 && filename.find("generic") != std::string::npos) {
            initrd_src = entry.path();
        }
    }

    if (vmlinuz_src.empty() || initrd_src.empty()) {
        LOG_ERROR("Could not locate generic kernel or initrd in rootfs /boot.");
        return false;
    }

    LOG_INFO("Copying Kernel: " + vmlinuz_src.filename().string() + " -> image/casper/vmlinuz");
    fs::copy_file(vmlinuz_src, img / "casper/vmlinuz", fs::copy_options::overwrite_existing, ec);
    if (ec) {
        LOG_ERROR("Failed to copy kernel: " + ec.message());
        return false;
    }

    LOG_INFO("Copying Initrd: " + initrd_src.filename().string() + " -> image/casper/initrd");
    fs::copy_file(initrd_src, img / "casper/initrd", fs::copy_options::overwrite_existing, ec);
    if (ec) {
        LOG_ERROR("Failed to copy initrd: " + ec.message());
        return false;
    }

    return true;
}

bool IsoGenerator::generate_grub_config(StepContext& ctx) {
    const auto& img = ctx.image_path();
    const auto& cfg = ctx.config();

    std::string try_text = "Try and Install " + cfg.business_name;

    std::string grub_cfg = 
        "search --set=root --file /" + cfg.target_name + "\n\n"
        "insmod all_video\n\n"
        "set default=\"0\"\n"
        "set timeout=10\n\n"
        "menuentry \"" + try_text + "\" {\n"
        "   set gfxpayload=keep\n"
        "   linux   /casper/vmlinuz boot=casper nopersistent quiet splash ---\n"
        "   initrd  /casper/initrd\n"
        "}\n\n"
        "menuentry \"" + try_text + " (Safe Graphics)\" {\n"
        "   set gfxpayload=keep\n"
        "   linux   /casper/vmlinuz boot=casper nopersistent nomodeset ---\n"
        "   initrd  /casper/initrd\n"
        "}\n\n"
        "if [ \"$grub_platform\" == \"efi\" ]; then\n"
        "    menuentry \"Boot from next volume\" {\n"
        "        exit 1\n"
        "    }\n\n"
        "    menuentry \"UEFI Firmware Settings\" {\n"
        "        fwsetup\n"
        "    }\n"
        "fi\n";

    fs::path marker_file = img / cfg.target_name;
    FileOps::write_file_atomic(marker_file, "# VAXP-OS Live Marker\n");

    return FileOps::write_file_atomic(img / "isolinux/grub.cfg", grub_cfg);
}

bool IsoGenerator::generate_manifests(StepContext& ctx) {
    const auto& img    = ctx.image_path();
    const auto& cfg    = ctx.config();

    LOG_INFO("Generating packages manifest...");

    auto res = ctx.run_chroot("dpkg-query", {"-W", "--showformat=${Package} ${Version}\n"});
    if (!res.success()) {
        LOG_ERROR("Failed to query package manifest from rootfs.");
        return false;
    }

    FileOps::write_file_atomic(img / "casper/filesystem.manifest", res.stdout_output);

    // Create desktop manifest (filter out removed packages)
    std::string desktop_manifest;
    std::stringstream ss(res.stdout_output);
    std::string line;

    while (std::getline(ss, line)) {
        bool should_remove = false;
        for (const auto& pkg : cfg.packages_to_remove) {
            if (line.rfind(pkg, 0) == 0) {
                should_remove = true;
                break;
            }
        }
        if (!should_remove) {
            desktop_manifest += line + "\n";
        }
    }

    FileOps::write_file_atomic(img / "casper/filesystem.manifest-desktop", desktop_manifest);
    LOG_OK("Manifests generated.");
    return true;
}

bool IsoGenerator::generate_disk_metadata(StepContext& ctx) {
    const auto& img = ctx.image_path();
    const auto& cfg = ctx.config();
    std::string date_str = get_current_date_str();

    std::string diskdefines =
        "#define DISKNAME  Try " + cfg.business_name + "\n"
        "#define TYPE  binary\n"
        "#define TYPEbinary  1\n"
        "#define ARCH  " + cfg.architecture + "\n"
        "#define ARCHamd64  1\n"
        "#define DISKNUM  1\n"
        "#define DISKNUM1  1\n"
        "#define TOTALNUM  0\n"
        "#define TOTALNUM0  1\n";

    FileOps::write_file_atomic(img / "README.diskdefines", diskdefines);

    std::string diskinfo = cfg.business_name + " " + cfg.version + " " + cfg.ubuntu_codename + 
                           " - Release " + cfg.architecture + " (" + date_str + ")\n";
    FileOps::write_file_atomic(img / ".disk/info", diskinfo);

    std::string readme_md =
        "# " + cfg.business_name + " " + cfg.version + "\n\n"
        + cfg.business_name + " is a commercial-grade, high performance operating system built for modern computing.\n\n"
        "- **Language**: " + cfg.lang_mode + "\n"
        "- **Version**: " + cfg.version + "\n"
        "- **Base**: Ubuntu " + cfg.ubuntu_codename + "\n"
        "- **Build Date**: " + date_str + "\n\n"
        "## Integrity Check\n\n"
        "Verify your download checksum via SHA256 before installation.\n";

    FileOps::write_file_atomic(img / "README.md", readme_md);

    return true;
}

bool IsoGenerator::build_efi_boot_image(StepContext& ctx) {
    const auto& img = ctx.image_path();
    fs::path isolinux_dir = img / "isolinux";
    fs::path efi_img_file = isolinux_dir / "efiboot.img";

    LOG_INFO("Creating UEFI boot image: " + efi_img_file.string());

    // Allocate 10MB zeroed file
    auto dd_res = ctx.run_host("dd", {"if=/dev/zero", "of=" + efi_img_file.string(), "bs=1M", "count=10"});
    if (!dd_res.success()) return false;

    auto mkfs_res = ctx.run_host("mkfs.vfat", {efi_img_file.string()});
    if (!mkfs_res.success()) return false;

    fs::path mnt_efi = isolinux_dir / "efi_temp";
    FileOps::ensure_directory_exists(mnt_efi);

    if (!ctx.mounts().mount_bind(efi_img_file, mnt_efi)) {
        // Fallback to loop mount if bind fails for regular file
        auto mnt_res = ctx.run_host("mount", {"-o", "loop", efi_img_file.string(), mnt_efi.string()});
        if (!mnt_res.success()) return false;
    }

    auto grub_res = ctx.run_host("grub-install", {
        "--efi-directory=" + mnt_efi.string(),
        "--uefi-secure-boot",
        "--removable",
        "--no-nvram"
    });

    ctx.mounts().unmount(mnt_efi);
    ctx.run_host("umount", {"-f", mnt_efi.string()});
    std::error_code ec;
    fs::remove(mnt_efi, ec);

    if (!grub_res.success()) {
        LOG_ERROR("grub-install for UEFI failed.");
        return false;
    }

    LOG_OK("UEFI boot image successfully generated.");
    return true;
}

bool IsoGenerator::build_bios_boot_image(StepContext& ctx) {
    const auto& img = ctx.image_path();
    fs::path isolinux_dir = img / "isolinux";
    fs::path core_img     = isolinux_dir / "core.img";
    fs::path bios_img     = isolinux_dir / "bios.img";
    fs::path grub_cfg     = isolinux_dir / "grub.cfg";

    LOG_INFO("Creating Legacy BIOS standalone GRUB image...");

    std::vector<std::string> args = {
        "--format=i386-pc",
        "--output=" + core_img.string(),
        "--install-modules=linux16 linux normal iso9660 biosdisk memdisk search tar ls",
        "--modules=linux16 linux normal iso9660 biosdisk search",
        "--locales=",
        "--fonts=",
        "boot/grub/grub.cfg=" + grub_cfg.string()
    };

    auto mk_res = ctx.run_host("grub-mkstandalone", args);
    if (!mk_res.success()) {
        LOG_ERROR("grub-mkstandalone failed.");
        return false;
    }

    // Combine /usr/lib/grub/i386-pc/cdboot.img + core.img -> bios.img
    fs::path cdboot_path = "/usr/lib/grub/i386-pc/cdboot.img";
    if (!fs::exists(cdboot_path)) {
        LOG_ERROR("Missing /usr/lib/grub/i386-pc/cdboot.img (grub-pc-bin required)");
        return false;
    }

    std::ifstream cdboot_in(cdboot_path, std::ios::binary);
    std::ifstream core_in(core_img, std::ios::binary);
    std::ofstream bios_out(bios_img, std::ios::binary | std::ios::trunc);

    if (!cdboot_in.is_open() || !core_in.is_open() || !bios_out.is_open()) {
        LOG_ERROR("Failed to combine cdboot.img and core.img into bios.img");
        return false;
    }

    bios_out << cdboot_in.rdbuf() << core_in.rdbuf();
    bios_out.close();

    LOG_OK("BIOS boot image created successfully.");
    return true;
}

bool IsoGenerator::generate_md5sums(StepContext& ctx) {
    const auto& img = ctx.image_path();
    LOG_INFO("Generating md5sum.txt index for image files...");

    std::ofstream md5_out(img / "md5sum.txt", std::ios::trunc);
    if (!md5_out.is_open()) return false;

    std::error_code ec;
    for (const auto& entry : fs::recursive_directory_iterator(img, ec)) {
        if (!entry.is_regular_file(ec)) continue;

        std::string filename = entry.path().filename().string();
        if (filename == "md5sum.txt" || filename == "bios.img" || filename == "efiboot.img") {
            continue;
        }

        std::string hash = FileOps::calculate_md5(entry.path());
        if (!hash.empty()) {
            fs::path rel = fs::relative(entry.path(), img, ec);
            md5_out << hash << "  ./" << rel.string() << "\n";
        }
    }

    return true;
}

bool IsoGenerator::master_iso(StepContext& ctx, fs::path& output_iso_path) {
    const auto& img = ctx.image_path();
    const auto& cfg = ctx.config();
    const auto& dist = ctx.dist_path();

    FileOps::ensure_directory_exists(dist);

    std::string date_str = get_current_date_str();
    std::string iso_filename = cfg.business_name + "-" + cfg.version + "-" + cfg.lang_mode + "-" + date_str + ".iso";
    output_iso_path = dist / iso_filename;

    LOG_INFO("Mastering hybrid bootable ISO using xorriso: " + output_iso_path.string());

    std::vector<std::string> args = {
        "-as", "mkisofs",
        "-iso-level", "3",
        "-full-iso9660-filenames",
        "-volid", cfg.target_name,
        "-eltorito-boot", "boot/grub/bios.img",
        "-no-emul-boot",
        "-boot-load-size", "4",
        "-boot-info-table",
        "--eltorito-catalog", "boot/grub/boot.cat",
        "--grub2-boot-info",
        "--grub2-mbr", "/usr/lib/grub/i386-pc/boot_hybrid.img",
        "-eltorito-alt-boot",
        "-e", "EFI/efiboot.img",
        "-no-emul-boot",
        "-append_partition", "2", "0xef", (img / "isolinux/efiboot.img").string(),
        "-output", output_iso_path.string(),
        "-m", "isolinux/efiboot.img",
        "-m", "isolinux/bios.img",
        "-graft-points",
        "/EFI/efiboot.img=" + (img / "isolinux/efiboot.img").string(),
        "/boot/grub/grub.cfg=" + (img / "isolinux/grub.cfg").string(),
        "/boot/grub/bios.img=" + (img / "isolinux/bios.img").string(),
        img.string()
    };

    ProcessOptions opts;
    opts.stream_output = true;
    opts.log_prefix = "xorriso";

    auto res = Process::run("xorriso", args, opts);
    if (!res.success()) {
        LOG_ERROR("xorriso ISO generation failed.");
        return false;
    }

    LOG_OK("ISO generated successfully: " + output_iso_path.string());

    // Calculate SHA256 checksum natively
    LOG_INFO("Calculating SHA256 hash...");
    std::string sha256_hash = FileOps::calculate_sha256(output_iso_path);
    if (!sha256_hash.empty()) {
        fs::path sha_path = dist / (output_iso_path.stem().string() + ".sha256");
        FileOps::write_file_atomic(sha_path, "SHA256: " + sha256_hash + "  " + iso_filename + "\n");
        LOG_OK("SHA256: " + sha256_hash);
    }

    return true;
}

bool IsoGenerator::generate(StepContext& ctx) {
    LOG_STEP("ISO", "Starting ISO image construction...");

    if (!prepare_image_layout(ctx)) return false;
    if (!copy_kernel_and_initrd(ctx)) return false;
    if (!generate_grub_config(ctx)) return false;
    if (!generate_manifests(ctx)) return false;

    // Compress rootfs to squashfs
    fs::path squashfs_dest = ctx.image_path() / "casper/filesystem.squashfs";
    if (!SquashFs::create(ctx.rootfs_path(), squashfs_dest)) return false;

    // Calculate and write filesystem.size
    SquashFs::calculate_and_write_size(ctx.rootfs_path(), ctx.image_path() / "casper/filesystem.size");

    if (!generate_disk_metadata(ctx)) return false;
    if (!build_efi_boot_image(ctx)) return false;
    if (!build_bios_boot_image(ctx)) return false;
    if (!generate_md5sums(ctx)) return false;

    fs::path output_iso;
    if (!master_iso(ctx, output_iso)) return false;

    LOG_OK("ISO Build Pipeline fully completed: " + output_iso.string());
    return true;
}

} // namespace vaxp
