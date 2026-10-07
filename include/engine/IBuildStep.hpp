#pragma once

#include "engine/StepContext.hpp"
#include <string>
#include <memory>

namespace vaxp {

class IBuildStep {
public:
    virtual ~IBuildStep() = default;

    [[nodiscard]] virtual std::string id() const = 0;
    [[nodiscard]] virtual std::string title() const = 0;
    [[nodiscard]] virtual std::string description() const = 0;
    [[nodiscard]] virtual bool requires_chroot_mounts() const = 0;

    virtual bool execute(StepContext& ctx) = 0;
};

using BuildStepPtr = std::unique_ptr<IBuildStep>;

} // namespace vaxp
