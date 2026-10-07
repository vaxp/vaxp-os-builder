#pragma once

#include "core/Types.hpp"
#include <string>
#include <vector>
#include <filesystem>
#include <sys/types.h>

namespace vaxp {

class FileOps {
public:
    static std::string calculate_sha256(const fs::path& file_path);
    static std::string calculate_md5(const fs::path& file_path);

    static bool write_file_atomic(const fs::path& file_path, 
                                  const std::string& content, 
                                  mode_t mode = 0644);

    static std::string read_file(const fs::path& file_path);

    static bool copy_directory_recursive(const fs::path& source, 
                                        const fs::path& destination);

    static bool remove_all_safe(const fs::path& target_path);

    static uint64_t get_available_disk_space(const fs::path& path);
    static uint64_t get_directory_size_bytes(const fs::path& dir_path);

    static bool ensure_directory_exists(const fs::path& dir_path);
};

} // namespace vaxp
