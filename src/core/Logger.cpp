#include "core/Logger.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <unistd.h>

namespace vaxp {

namespace {
    constexpr std::string_view COLOR_RESET   = "\033[0m";
    constexpr std::string_view COLOR_BOLD    = "\033[1m";
    constexpr std::string_view COLOR_GREEN   = "\033[1;32m";
    constexpr std::string_view COLOR_BLUE    = "\033[1;34m";
    constexpr std::string_view COLOR_CYAN    = "\033[1;36m";
    constexpr std::string_view COLOR_YELLOW  = "\033[1;33m";
    constexpr std::string_view COLOR_RED     = "\033[1;31m";
    constexpr std::string_view COLOR_MAGENTA = "\033[1;35m";
}

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

Logger::~Logger() {
    close();
}

void Logger::init(const fs::path& log_file_path, bool verbose) {
    std::lock_guard<std::mutex> lock(mutex_);
    verbose_ = verbose;
    
    if (file_stream_.is_open()) {
        file_stream_.close();
    }
    
    if (!log_file_path.empty()) {
        std::error_code ec;
        if (log_file_path.has_parent_path()) {
            fs::create_directories(log_file_path.parent_path(), ec);
        }
        file_stream_.open(log_file_path, std::ios::out | std::ios::app);
    }
    initialized_ = true;
}

void Logger::close() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_stream_.is_open()) {
        file_stream_.flush();
        file_stream_.close();
    }
}

std::string Logger::current_time_str() const {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    struct tm tm_buf{};
    localtime_r(&in_time_t, &tm_buf);
    ss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void Logger::log(LogLevel level, std::string_view message) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (level == LogLevel::DEBUG && !verbose_) {
        return;
    }

    std::string time_str = current_time_str();
    std::string level_tag;
    std::string color_code;

    switch (level) {
        case LogLevel::DEBUG:
            level_tag = "[DEBUG]";
            color_code = COLOR_MAGENTA;
            break;
        case LogLevel::INFO:
            level_tag = "[ INFO]";
            color_code = COLOR_CYAN;
            break;
        case LogLevel::SUCCESS:
            level_tag = "[  OK ]";
            color_code = COLOR_GREEN;
            break;
        case LogLevel::WARN:
            level_tag = "[ WARN]";
            color_code = COLOR_YELLOW;
            break;
        case LogLevel::ERROR:
            level_tag = "[FAILED]";
            color_code = COLOR_RED;
            break;
        case LogLevel::STEP:
            level_tag = "[ STEP]";
            color_code = COLOR_BLUE;
            break;
    }

    // Print to stdout or stderr
    std::ostream& out = (level == LogLevel::ERROR) ? std::cerr : std::cout;
    
    bool is_tty = isatty(fileno((level == LogLevel::ERROR) ? stderr : stdout)) != 0;
    if (is_tty) {
        out << COLOR_BOLD << "[" << time_str << "] " << COLOR_RESET
            << color_code << level_tag << COLOR_RESET << " "
            << message << "\n";
    } else {
        out << "[" << time_str << "] " << level_tag << " " << message << "\n";
    }
    out.flush();

    // Log to file without ANSI colors
    if (file_stream_.is_open()) {
        file_stream_ << "[" << time_str << "] " << level_tag << " " << message << "\n";
        file_stream_.flush();
    }
}

void Logger::debug(std::string_view message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(std::string_view message) {
    log(LogLevel::INFO, message);
}

void Logger::success(std::string_view message) {
    log(LogLevel::SUCCESS, message);
}

void Logger::warn(std::string_view message) {
    log(LogLevel::WARN, message);
}

void Logger::error(std::string_view message) {
    log(LogLevel::ERROR, message);
}

void Logger::step(std::string_view step_name, std::string_view message) {
    std::string full_msg = "[" + std::string(step_name) + "] " + std::string(message);
    log(LogLevel::STEP, full_msg);
}

} // namespace vaxp
