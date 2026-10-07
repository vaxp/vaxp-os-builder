#pragma once

#include "engine/IBuildStep.hpp"
#include <vector>
#include <unordered_set>
#include <string>
#include <filesystem>

namespace vaxp {

class StepPipeline {
public:
    StepPipeline(const fs::path& state_file_path);

    void register_step(BuildStepPtr step);

    bool execute_all(StepContext& ctx, bool resume_state);
    bool execute_single(const std::string& step_id, StepContext& ctx);

    void list_steps() const;
    void reset_state();

private:
    void load_completed_steps();
    void mark_step_completed(const std::string& step_id);
    [[nodiscard]] bool is_step_completed(const std::string& step_id) const;

    fs::path state_file_path_;
    std::vector<BuildStepPtr> steps_;
    std::unordered_set<std::string> completed_steps_;
};

} // namespace vaxp
