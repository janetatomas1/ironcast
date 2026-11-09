#include <mutex>
#include <vector>

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "ironcast/log.hpp"

namespace ironcast {
    namespace {
        constexpr const char* ironcastName = "ironcast";
        constexpr const char* appName = "app";

        // Guards the shared sinks and config; spdlog's sinks are thread-safe (_mt) themselves.
        std::mutex mutex;
        std::vector<spdlog::sink_ptr> sinks;
        LogConfig current;
        std::shared_ptr<MemorySink> memory;
        bool configured = false;

        std::vector<spdlog::sink_ptr> make_sinks(const LogConfig& config) {
            std::vector<spdlog::sink_ptr> result;

            if (config.console) {
                result.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
            }

            if (!config.file.empty()) {
                result.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                    config.file.string(), config.maxFileSize, config.maxFiles
                ));
            }

            // Kept across configs, so the messages logged so far stay.
            if (config.memoryCapacity == 0) {
                memory.reset();
            } else if (memory == nullptr) {
                memory = std::make_shared<MemorySink>(config.memoryCapacity);
            } else {
                memory->set_capacity(config.memoryCapacity);
            }

            if (memory != nullptr) {
                result.push_back(memory);
            }
            return result;
        }

        // Points a logger at the shared sinks and the config's level for its kind.
        void apply(spdlog::logger& logger) {
            logger.sinks() = sinks;
            logger.set_level(logger.name() == ironcastName ? current.ironcastLevel : current.level);
            logger.set_pattern(current.pattern);
            // Files are written in chunks; make sure problems reach the disk right away.
            logger.flush_on(spdlog::level::warn);
        }

        void configure_locked(const LogConfig& config) {
            current = config;
            sinks = make_sinks(config);
            configured = true;

            if (spdlog::get(ironcastName) == nullptr) {
                spdlog::register_logger(std::make_shared<spdlog::logger>(ironcastName));
            }

            if (spdlog::default_logger()->name() != appName) {
                auto app = std::make_shared<spdlog::logger>(appName);
                spdlog::set_default_logger(app);
            }
            spdlog::apply_all([](const std::shared_ptr<spdlog::logger>& logger) {
                apply(*logger);
            });
        }
    }

    MemorySink::MemorySink(std::size_t capacity): capacity_(capacity) {}

    void MemorySink::sink_it_(const spdlog::details::log_msg& msg) {
        spdlog::memory_buf_t buffer;
        formatter_->format(msg, buffer);
        std::string line(buffer.data(), buffer.size());

        while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
            line.pop_back();
        }
        entries_.push_back({msg.level, std::string(msg.logger_name.data(), msg.logger_name.size()), std::move(line)});

        while (entries_.size() > capacity_) {
            entries_.pop_front();
        }
        ++version_;

        if (onMessage_) {
            onMessage_();
        }
    }

    std::uint64_t MemorySink::version() const {
        return version_;
    }

    void MemorySink::copy(std::vector<LogEntry>& out) {
        std::lock_guard lock(mutex_);
        out.assign(entries_.begin(), entries_.end());
    }

    void MemorySink::clear() {
        std::lock_guard lock(mutex_);
        entries_.clear();
        ++version_;
    }

    std::size_t MemorySink::capacity() {
        std::lock_guard lock(mutex_);
        return capacity_;
    }

    void MemorySink::set_capacity(std::size_t capacity) {
        std::lock_guard lock(mutex_);
        capacity_ = capacity;

        while (entries_.size() > capacity_) {
            entries_.pop_front();
        }
    }

    void MemorySink::set_on_message(std::function<void()> callback) {
        std::lock_guard lock(mutex_);
        onMessage_ = std::move(callback);
    }

    std::shared_ptr<MemorySink> log_memory() {
        std::lock_guard lock(mutex);

        if (!configured) {
            configure_locked({});
        }
        return memory;
    }

    spdlog::logger& logger() {
        std::lock_guard lock(mutex);

        if (!configured) {
            configure_locked({});
        }
        return *spdlog::get(ironcastName);
    }

    void configure_logging(const LogConfig& config) {
        std::lock_guard lock(mutex);
        configure_locked(config);
    }

    std::shared_ptr<spdlog::logger> make_logger(const std::string& name) {
        std::lock_guard lock(mutex);

        if (!configured) {
            configure_locked({});
        }

        if (auto existing = spdlog::get(name)) {
            return existing;
        }

        auto result = std::make_shared<spdlog::logger>(name);
        apply(*result);
        spdlog::register_logger(result);

        return result;
    }
}
