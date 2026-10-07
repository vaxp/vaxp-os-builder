#pragma once

#include <atomic>
#include <vector>
#include <functional>
#include <mutex>

namespace vaxp {

class SignalHandler {
public:
    using CleanupCallback = std::function<void()>;

    static SignalHandler& instance();

    void initialize();
    void register_cleanup(CleanupCallback callback);
    void unregister_all_cleanups();

    [[nodiscard]] bool is_interrupted() const noexcept {
        return interrupted_.load(std::memory_order_relaxed);
    }

    void set_interrupted(bool val = true) noexcept {
        interrupted_.store(val, std::memory_order_relaxed);
    }

    void trigger_cleanup();

private:
    SignalHandler() = default;
    ~SignalHandler() = default;
    SignalHandler(const SignalHandler&) = delete;
    SignalHandler& operator=(const SignalHandler&) = delete;

    static void handle_posix_signal(int sig);

    std::atomic<bool> interrupted_{false};
    std::mutex mutex_;
    std::vector<CleanupCallback> cleanups_;
};

} // namespace vaxp
