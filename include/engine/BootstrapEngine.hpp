#pragma once

#include "engine/StepContext.hpp"

namespace vaxp {

class BootstrapEngine {
public:
    static bool bootstrap(StepContext& ctx);
    static bool is_bootstrapped(const fs::path& rootfs_path);
};

} // namespace vaxp
