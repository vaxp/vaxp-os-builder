#include "core/Process.hpp"
#include "core/Logger.hpp"
#include "core/SignalHandler.hpp"

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

#include <iostream>
#include <sstream>
#include <cstring>
#include <cerrno>

namespace vaxp {

namespace {

void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags != -1) {
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
}

void process_buffer(std::string& accumulator, std::string& full_output, 
                    bool is_stderr, bool stream, const std::string& prefix) {
    size_t pos = 0;
    while ((pos = accumulator.find('\n')) != std::string::npos) {
        std::string line = accumulator.substr(0, pos);
        accumulator.erase(0, pos + 1);

        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (stream) {
            std::string formatted = prefix.empty() ? line : ("[" + prefix + "] " + line);
            if (is_stderr) {
                LOG_WARN(formatted);
            } else {
                LOG_INFO(formatted);
            }
        }

        full_output += line + "\n";
    }
}

} // anonymous namespace

ProcessResult Process::run(const std::string& command,
                           const std::vector<std::string>& args,
                           const ProcessOptions& options) {
    if (SignalHandler::instance().is_interrupted()) {
        return ProcessResult{.exit_code = -1, .stdout_output = "", .stderr_output = "Interrupted by user signal"};
    }

    int stdout_pipe[2];
    int stderr_pipe[2];

    if (pipe(stdout_pipe) < 0 || pipe(stderr_pipe) < 0) {
        std::string err = "Failed to create pipes: " + std::string(strerror(errno));
        LOG_ERROR(err);
        return ProcessResult{.exit_code = -1, .stdout_output = "", .stderr_output = err};
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(stdout_pipe[0]); close(stdout_pipe[1]);
        close(stderr_pipe[0]); close(stderr_pipe[1]);
        std::string err = "Fork failed: " + std::string(strerror(errno));
        LOG_ERROR(err);
        return ProcessResult{.exit_code = -1, .stdout_output = "", .stderr_output = err};
    }

    if (pid == 0) {
        // Child Process
        close(stdout_pipe[0]);
        close(stderr_pipe[0]);

        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);

        close(stdout_pipe[1]);
        close(stderr_pipe[1]);

        // Change working directory if requested
        if (options.working_directory) {
            if (chdir(options.working_directory->c_str()) != 0) {
                perror("chdir failed in child");
                _exit(127);
            }
        }

        // Apply environment variables
        for (const auto& [key, val] : options.environment) {
            setenv(key.c_str(), val.c_str(), 1);
        }

        // Build argv
        std::vector<char*> argv;
        argv.push_back(const_cast<char*>(command.c_str()));
        for (const auto& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr);

        execvp(command.c_str(), argv.data());

        // If execvp returns, it failed
        perror("execvp failed");
        _exit(127);
    }

    // Parent Process
    close(stdout_pipe[1]);
    close(stderr_pipe[1]);

    set_nonblocking(stdout_pipe[0]);
    set_nonblocking(stderr_pipe[0]);

    std::string stdout_acc, stderr_acc;
    std::string total_stdout, total_stderr;

    struct pollfd fds[2];
    fds[0].fd = stdout_pipe[0];
    fds[0].events = POLLIN;
    fds[1].fd = stderr_pipe[0];
    fds[1].events = POLLIN;

    char buffer[4096];
    bool stdout_open = true;
    bool stderr_open = true;

    while (stdout_open || stderr_open) {
        if (SignalHandler::instance().is_interrupted()) {
            kill(pid, SIGTERM);
            usleep(200000);
            kill(pid, SIGKILL);
            break;
        }

        int ret = poll(fds, 2, 100);
        if (ret < 0) {
            if (errno == EINTR) continue;
            break;
        }

        if (stdout_open && (fds[0].revents & (POLLIN | POLLHUP))) {
            ssize_t bytes = read(stdout_pipe[0], buffer, sizeof(buffer) - 1);
            if (bytes > 0) {
                buffer[bytes] = '\0';
                stdout_acc.append(buffer, bytes);
                process_buffer(stdout_acc, total_stdout, false, options.stream_output, options.log_prefix);
            } else if (bytes == 0 || (fds[0].revents & POLLHUP)) {
                stdout_open = false;
            }
        }

        if (stderr_open && (fds[1].revents & (POLLIN | POLLHUP))) {
            ssize_t bytes = read(stderr_pipe[0], buffer, sizeof(buffer) - 1);
            if (bytes > 0) {
                buffer[bytes] = '\0';
                stderr_acc.append(buffer, bytes);
                process_buffer(stderr_acc, total_stderr, true, options.stream_output, options.log_prefix);
            } else if (bytes == 0 || (fds[1].revents & POLLHUP)) {
                stderr_open = false;
            }
        }
    }

    // Process any remaining bytes
    if (!stdout_acc.empty()) {
        total_stdout += stdout_acc;
        if (options.stream_output) {
            LOG_INFO(stdout_acc);
        }
    }
    if (!stderr_acc.empty()) {
        total_stderr += stderr_acc;
        if (options.stream_output) {
            LOG_WARN(stderr_acc);
        }
    }

    close(stdout_pipe[0]);
    close(stderr_pipe[0]);

    int status = 0;
    waitpid(pid, &status, 0);

    int exit_code = -1;
    if (WIFEXITED(status)) {
        exit_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        exit_code = 128 + WTERMSIG(status);
    }

    return ProcessResult{
        .exit_code = exit_code,
        .stdout_output = options.capture_output ? total_stdout : "",
        .stderr_output = options.capture_output ? total_stderr : ""
    };
}

ProcessResult Process::run_shell(const std::string& shell_command,
                                const ProcessOptions& options) {
    return run("/bin/bash", {"-c", shell_command}, options);
}

ProcessResult Process::run_in_chroot(const fs::path& rootfs_path,
                                    const std::string& command,
                                    const std::vector<std::string>& args,
                                    const ProcessOptions& options) {
    std::vector<std::string> chroot_args = {rootfs_path.string(), command};
    chroot_args.insert(chroot_args.end(), args.begin(), args.end());
    return run("chroot", chroot_args, options);
}

ProcessResult Process::run_shell_in_chroot(const fs::path& rootfs_path,
                                          const std::string& shell_command,
                                          const ProcessOptions& options) {
    return run("chroot", {rootfs_path.string(), "/bin/bash", "-c", shell_command}, options);
}

} // namespace vaxp
