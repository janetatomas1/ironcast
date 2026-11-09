#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <spdlog/sinks/base_sink.h>
#include <spdlog/spdlog.h>

namespace ironcast {
    // Where log messages go. All loggers share the same sinks: ironcast's ("ironcast"), the app's default
    // logger ("app", spdlog's default) and every logger made with make_logger().
    struct LogConfig {
        // Level of the app's loggers (the default logger and make_logger() ones).
        spdlog::level::level_enum level = spdlog::level::info;
        // Level of the framework's "ironcast" logger, set apart so it can be quieted on its own.
        spdlog::level::level_enum ironcastLevel = spdlog::level::info;
        bool console = true;
        // Rotating log file; empty = none.
        std::filesystem::path file;
        std::size_t maxFileSize = 5 * 1024 * 1024;
        std::size_t maxFiles = 3;
        std::string pattern = "[%H:%M:%S.%e] [%n] [%^%l%$] %v";
        // Messages kept in memory for LogWidget, the oldest dropped first; 0 = none. The buffer lives from the
        // first configure_logging() on and keeps its messages when the config changes.
        std::size_t memoryCapacity = 5000;
    };

    // One message kept by MemorySink.
    struct LogEntry {
        spdlog::level::level_enum level;
        std::string logger;
        // Formatted with LogConfig::pattern, without the trailing newline.
        std::string line;
    };

    // Keeps the last capacity() messages of every logger. Thread-safe: loggers write from any thread.
    class MemorySink : public spdlog::sinks::base_sink<std::mutex> {
        std::deque<LogEntry> entries_;
        std::size_t capacity_;
        std::atomic<std::uint64_t> version_ = 0;
        std::function<void()> onMessage_;

    protected:
        void sink_it_(const spdlog::details::log_msg& msg) override;
        void flush_() override {}

    public:
        explicit MemorySink(std::size_t capacity);

        // Bumped by every message and by clear(); compare it to skip copying when nothing changed.
        [[nodiscard]] std::uint64_t version() const;
        // Replaces out with the kept messages, oldest first.
        void copy(std::vector<LogEntry>& out);
        void clear();

        [[nodiscard]] std::size_t capacity();
        void set_capacity(std::size_t capacity);
        // Called after each message, on the logging thread, with the sink locked. Ironcast uses it to wake the loop.
        void set_on_message(std::function<void()> callback);
    };

    // The in-memory buffer, or nullptr when LogConfig::memoryCapacity is 0.
    [[nodiscard]] std::shared_ptr<MemorySink> log_memory();

    // The framework's logger. Usable before the app exists; logs to the console until configured.
    [[nodiscard]] spdlog::logger& logger();

    // Applies config to every registered logger (sinks, levels, pattern). Ironcast calls it with the defaults on
    // construction; call it again (or Ironcast::set_log_config) to change it.
    void configure_logging(const LogConfig& config);

    // A named app logger sharing the configured sinks, e.g. make_logger("zion.cpu"). Registered with spdlog,
    // so spdlog::get(name) finds it too.
    std::shared_ptr<spdlog::logger> make_logger(const std::string& name);
}
