#pragma once

#include "core/Types.hpp"
#include <string>
#include <string_view>
#include <mutex>
#include <fstream>
#include <iostream>

namespace vaxp {

class Logger {
public:
    static Logger& instance();

    void init(const fs::path& log_file_path = "vaxp-builder.log", bool verbose = false);
    void close();

    void log(LogLevel level, std::string_view message);
    
    void debug(std::string_view message);
    void info(std::string_view message);
    void success(std::string_view message);
    void warn(std::string_view message);
    void error(std::string_view message);
    void step(std::string_view step_name, std::string_view message);

    void set_verbose(bool v) noexcept { verbose_ = v; }
    [[nodiscard]] bool is_verbose() const noexcept { return verbose_; }

private:
    Logger() = default;
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string current_time_str() const;

    std::mutex mutex_;
    std::ofstream file_stream_;
    bool verbose_{false};
    bool initialized_{false};
};

#define LOG_DEBUG(msg)   ::vaxp::Logger::instance().debug(msg)
#define LOG_INFO(msg)    ::vaxp::Logger::instance().info(msg)
#define LOG_OK(msg)      ::vaxp::Logger::instance().success(msg)
#define LOG_WARN(msg)    ::vaxp::Logger::instance().warn(msg)
#define LOG_ERROR(msg)   ::vaxp::Logger::instance().error(msg)
#define LOG_STEP(s, msg) ::vaxp::Logger::instance().step(s, msg)

} // namespace vaxp
