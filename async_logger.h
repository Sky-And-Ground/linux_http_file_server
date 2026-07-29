#pragma once

#include <fstream>
#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cstdarg>
#include "utils.h"

namespace async_log {
    enum class Level {
        debug,
        warn,
        info,
        error,
        fatal
    };

    const char* level_str(Level level) noexcept;

    /*
        LogQueue's destructor just do nothing,
        so its shutdown method must be called by the user.
    */
    class LogQueue {
        std::queue<std::string> messages;
        std::mutex mut;
        std::condition_variable cv;
        bool running = true;
    public:
        void push(const std::string& msg);
        bool pop(std::string& outMsg);
        void shutdown();
    };

    // a async logger.
    class Logger {
        std::ofstream stream;
        std::thread workThread;
        LogQueue logQueue;
        Level level = Level::info;

        Logger();

        void process_queue();

        void log(const char* fileName, const char* funcName, int lineNumber, Level lev, const std::string& msg);

        bool check_log_level(Level tmp) noexcept;
    public:
        static Logger& instance() {
            static Logger logger;
            return logger;
        }

        void open_file(const std::string& logPath);

        ~Logger();

        void set_level(Level _level) noexcept {
            level = _level;
        }

        template<typename ... Args>
        void simple_log(const char* fmt, Args ... args) {
            std::string tmp = utils::str_format(fmt, args...);
            logQueue.push(tmp);
        }

        template<typename ... Args>
        void debug(const char* fileName, const char* funcName, int lineNumber, const char* fmt, Args ... args) {
            if (check_log_level(Level::debug)) {
                log(fileName, funcName, lineNumber, Level::debug, utils::str_format(fmt, args...));
            }
        }

        template<typename ... Args>
        void warn(const char* fileName, const char* funcName, int lineNumber, const char* fmt, Args ... args) {
            if (check_log_level(Level::warn)) {
                log(fileName, funcName, lineNumber, Level::warn, utils::str_format(fmt, args...));
            }
        }

        template<typename ... Args>
        void info(const char* fileName, const char* funcName, int lineNumber, const char* fmt, Args ... args) {
            if (check_log_level(Level::info)) {
                log(fileName, funcName, lineNumber, Level::info, utils::str_format(fmt, args...));
            }
        }

        template<typename ... Args>
        void error(const char* fileName, const char* funcName, int lineNumber, const char* fmt, Args ... args) {
            if (check_log_level(Level::error)) {
                log(fileName, funcName, lineNumber, Level::error, utils::str_format(fmt, args...));
            }
        }

        template<typename ... Args>
        void fatal(const char* fileName, const char* funcName, int lineNumber, const char* fmt, Args ... args) {
            if (check_log_level(Level::fatal)) {
                log(fileName, funcName, lineNumber, Level::fatal, utils::str_format(fmt, args...));
            }
        }
    };
}

#define __LOG_FFL__    __FILE__, __func__, __LINE__

#define SIMPLE_LOG(...) async_log::Logger::instance().simple_log(__VA_ARGS__)
#define LOG_DEBUG(...)  async_log::Logger::instance().debug(__LOG_FFL__, __VA_ARGS__)
#define LOG_WARN(...)   async_log::Logger::instance().warn(__LOG_FFL__, __VA_ARGS__)
#define LOG_INFO(...)   async_log::Logger::instance().info(__LOG_FFL__, __VA_ARGS__)
#define LOG_ERROR(...)  async_log::Logger::instance().error(__LOG_FFL__, __VA_ARGS__)
#define LOG_FATAL(...)  async_log::Logger::instance().fatal(__LOG_FFL__, __VA_ARGS__)