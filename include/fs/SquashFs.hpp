#pragma once

#include "core/Types.hpp"
#include <filesystem>
#include <string>
#include <vector>

namespace vaxp {

struct SquashFsOptions {
    std::string compression{"zstd"};
    int compression_level{19};
    std::string block_size{"1M"};
    std::vector<std::string> exclusions{
        "var/cache/apt/archives/*",
        "root/*",
        "root/.*",
        "tmp/*",
        "tmp/.*",
        "swapfile"
    };
};

class SquashFs {
public:
    static bool create(const fs::path& source_dir,
                       const fs::path& destination_file,
                       const SquashFsOptions& options = {});

    static bool verify_integrity(const fs::path& squashfs_file);

    static uint64_t calculate_and_write_size(const fs::path& source_dir,
                                            const fs::path& size_file_destination);
};

} // namespace vaxp
