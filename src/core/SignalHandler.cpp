#include "core/SignalHandler.hpp"
#include "core/Logger.hpp"
#include <csignal>
#include <unistd.h>

namespace vaxp {

SignalHandler& SignalHandler::instance() {
    static SignalHandler sh;
    return sh;
}

void SignalHandler::handle_posix_signal(int sig) {
    SignalHandler::instance().set_interrupted(true);
    
    // We can notify on stderr safely using write
    const char msg[] = "\n[SIGNAL] Received termination signal! Cleaning up resources...\n";
    [[maybe_unused]] auto res = write(STDERR_FILENO, msg, sizeof(msg) - 1);

    SignalHandler::instance().trigger_cleanup();

    // Re-raise with default action or exit
    _exit(128 + sig);
}

void SignalHandler::initialize() {
    struct sigaction sa{};
    sa.sa_handler = SignalHandler::handle_posix_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESETHAND; // Ensure signal reset to prevent loops

    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGHUP, &sa, nullptr);
}

void SignalHandler::register_cleanup(CleanupCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    cleanups_.push_back(std::move(callback));
}

void SignalHandler::unregister_all_cleanups() {
    std::lock_guard<std::mutex> lock(mutex_);
    cleanups_.clear();
}

void SignalHandler::trigger_cleanup() {
    std::vector<CleanupCallback> copy;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        copy = cleanups_;
    }
    // Execute cleanups in reverse order of registration
    for (auto it = copy.rbegin(); it != copy.rend(); ++it) {
        try {
            if (*it) {
                (*it)();
            }
        } catch (...) {
            // Ignore exceptions in signal cleanup
        }
    }
}

} // namespace vaxp
