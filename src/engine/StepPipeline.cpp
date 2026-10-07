#include "engine/StepPipeline.hpp"
#include "core/Logger.hpp"
#include "core/SignalHandler.hpp"

#include <fstream>
#include <iostream>
#include <iomanip>

namespace vaxp {

StepPipeline::StepPipeline(const fs::path& state_file_path)
    : state_file_path_(state_file_path) {
    load_completed_steps();
}

void StepPipeline::register_step(BuildStepPtr step) {
    steps_.push_back(std::move(step));
}

void StepPipeline::load_completed_steps() {
    completed_steps_.clear();
    if (!fs::exists(state_file_path_)) return;

    std::ifstream in(state_file_path_);
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line[0] != '#') {
            completed_steps_.insert(line);
        }
    }
}

void StepPipeline::mark_step_completed(const std::string& step_id) {
    completed_steps_.insert(step_id);
    std::ofstream out(state_file_path_, std::ios::app);
    if (out.is_open()) {
        out << step_id << "\n";
    }
}

bool StepPipeline::is_step_completed(const std::string& step_id) const {
    return completed_steps_.contains(step_id);
}

void StepPipeline::reset_state() {
    std::error_code ec;
    fs::remove(state_file_path_, ec);
    completed_steps_.clear();
    LOG_INFO("Pipeline build state reset.");
}

void StepPipeline::list_steps() const {
    std::cout << "\n=======================================================\n"
              << "            VAXP-OS Pipeline Registered Steps           \n"
              << "=======================================================\n";
    for (size_t i = 0; i < steps_.size(); ++i) {
        const auto& s = steps_[i];
        bool done = is_step_completed(s->id());
        std::cout << " [" << std::right << std::setw(2) << std::setfill('0') << (i + 1) << "] "
                  << std::left << std::setfill(' ') << std::setw(25) << s->id()
                  << " | " << (done ? "[COMPLETED]" : "[ PENDING ]")
                  << " | " << s->title() << "\n";
    }
    std::cout << "=======================================================\n\n";
}

bool StepPipeline::execute_single(const std::string& step_id, StepContext& ctx) {
    for (auto& s : steps_) {
        if (s->id() == step_id) {
            LOG_STEP(s->id(), "Executing single step: " + s->title());
            
            bool mounts_needed = s->requires_chroot_mounts();
            if (mounts_needed) {
                if (!ctx.mounts().mount_all(ctx.rootfs_path())) {
                    LOG_ERROR("Failed to mount virtual filesystems for step: " + s->id());
                    return false;
                }
            }

            auto start = std::chrono::steady_clock::now();
            bool ok = s->execute(ctx);
            auto end = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();

            if (mounts_needed) {
                ctx.mounts().unmount_all(ctx.rootfs_path());
            }

            if (ok) {
                LOG_OK("Step [" + s->id() + "] completed in " + std::to_string(duration) + "s");
                mark_step_completed(s->id());
                return true;
            } else {
                LOG_ERROR("Step [" + s->id() + "] failed after " + std::to_string(duration) + "s");
                return false;
            }
        }
    }
    LOG_ERROR("Step not found: " + step_id);
    return false;
}

bool StepPipeline::execute_all(StepContext& ctx, bool resume_state) {
    LOG_INFO("Starting pipeline execution (" + std::to_string(steps_.size()) + " total steps)");

    size_t count = 0;
    for (auto& s : steps_) {
        if (SignalHandler::instance().is_interrupted()) {
            LOG_WARN("Pipeline interrupted by signal.");
            return false;
        }

        count++;
        if (resume_state && is_step_completed(s->id())) {
            LOG_INFO("Skipping previously completed step [" + std::to_string(count) + "/" + 
                     std::to_string(steps_.size()) + "]: " + s->id());
            continue;
        }

        LOG_STEP(s->id(), "Running [" + std::to_string(count) + "/" + 
                          std::to_string(steps_.size()) + "]: " + s->title());

        bool mounts_needed = s->requires_chroot_mounts();
        if (mounts_needed && !ctx.mounts().is_mounted(ctx.rootfs_path() / "dev")) {
            if (!ctx.mounts().mount_all(ctx.rootfs_path())) {
                LOG_ERROR("Failed to mount virtual filesystems for step: " + s->id());
                return false;
            }
        }

        auto start = std::chrono::steady_clock::now();
        bool success = false;
        try {
            success = s->execute(ctx);
        } catch (const std::exception& ex) {
            LOG_ERROR("Exception caught in step [" + s->id() + "]: " + ex.what());
            success = false;
        } catch (...) {
            LOG_ERROR("Unknown exception caught in step [" + s->id() + "]");
            success = false;
        }
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();

        if (!success) {
            LOG_ERROR("Execution halted due to failure in step: " + s->id() + " (" + s->title() + ")");
            if (mounts_needed) {
                ctx.mounts().unmount_all(ctx.rootfs_path());
            }
            return false;
        }

        mark_step_completed(s->id());
        LOG_OK("Finished step [" + s->id() + "] (" + std::to_string(duration) + "s)");
    }

    // Ensure all unmounted at pipeline completion
    ctx.mounts().unmount_all(ctx.rootfs_path());

    LOG_OK("All pipeline steps completed successfully!");
    return true;
}

} // namespace vaxp
