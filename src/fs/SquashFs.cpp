#include "fs/SquashFs.hpp"
#include "core/Process.hpp"
#include "core/Logger.hpp"
#include "fs/FileOps.hpp"

#include <fstream>

namespace vaxp {

bool SquashFs::create(const fs::path& source_dir,
                      const fs::path& destination_file,
                      const SquashFsOptions& options) {
    LOG_INFO("Creating SquashFS archive: " + destination_file.string());

    if (destination_file.has_parent_path()) {
        FileOps::ensure_directory_exists(destination_file.parent_path());
    }

    // Delete existing file if present
    std::error_code ec;
    fs::remove(destination_file, ec);

    std::vector<std::string> args = {
        source_dir.string(),
        destination_file.string(),
        "-noappend",
        "-no-duplicates",
        "-no-recovery",
        "-wildcards",
        "-b", options.block_size,
        "-comp", options.compression,
        "-Xcompression-level", std::to_string(options.compression_level)
    };

    for (const auto& excl : options.exclusions) {
        args.push_back("-e");
        args.push_back(excl);
    }

    ProcessOptions p_opts;
    p_opts.stream_output = true;
    p_opts.log_prefix = "mksquashfs";

    auto res = Process::run("mksquashfs", args, p_opts);
    if (!res.success()) {
        LOG_ERROR("mksquashfs execution failed with code: " + std::to_string(res.exit_code));
        return false;
    }

    LOG_OK("SquashFS image successfully created.");
    return verify_integrity(destination_file);
}

bool SquashFs::verify_integrity(const fs::path& squashfs_file) {
    LOG_INFO("Verifying integrity of SquashFS image: " + squashfs_file.string());
    
    ProcessOptions p_opts;
    p_opts.stream_output = false;
    p_opts.capture_output = true;

    auto res = Process::run("unsquashfs", {"-s", squashfs_file.string()}, p_opts);
    if (!res.success()) {
        LOG_ERROR("SquashFS verification FAILED! Corrupt or invalid image.");
        return false;
    }

    LOG_OK("SquashFS integrity verified successfully.");
    return true;
}

uint64_t SquashFs::calculate_and_write_size(const fs::path& source_dir,
                                           const fs::path& size_file_destination) {
    LOG_INFO("Calculating rootfs size for filesystem.size...");

    // Execute du -sx --block-size=1
    ProcessOptions p_opts;
    p_opts.stream_output = false;
    p_opts.capture_output = true;

    auto res = Process::run("du", {"-sx", "--block-size=1", source_dir.string()}, p_opts);
    if (!res.success()) {
        LOG_ERROR("Failed to compute filesystem size via du.");
        return 0;
    }

    std::stringstream ss(res.stdout_output);
    uint64_t bytes = 0;
    ss >> bytes;

    std::string byte_str = std::to_string(bytes);
    if (FileOps::write_file_atomic(size_file_destination, byte_str)) {
        LOG_OK("filesystem.size written (" + byte_str + " bytes)");
    } else {
        LOG_ERROR("Failed to write filesystem.size file.");
    }
    return bytes;
}

} // namespace vaxp
