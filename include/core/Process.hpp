#pragma once

#include "core/Types.hpp"
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <filesystem>

namespace vaxp {

struct ProcessOptions {
    std::optional<fs::path> working_directory;
    std::optional<fs::path> chroot_directory;
    std::map<std::string, std::string> environment;
    bool stream_output{true};
    bool capture_output{true};
    std::string log_prefix;
    int timeout_seconds{0}; // 0 = no timeout
};

class Process {
public:
    static ProcessResult run(const std::string& command,
                            const std::vector<std::string>& args = {},
                            const ProcessOptions& options = {});

    static ProcessResult run_shell(const std::string& shell_command,
                                  const ProcessOptions& options = {});

    static ProcessResult run_in_chroot(const fs::path& rootfs_path,
                                      const std::string& command,
                                      const std::vector<std::string>& args = {},
                                      const ProcessOptions& options = {});

    static ProcessResult run_shell_in_chroot(const fs::path& rootfs_path,
                                            const std::string& shell_command,
                                            const ProcessOptions& options = {});
};

} // namespace vaxp
