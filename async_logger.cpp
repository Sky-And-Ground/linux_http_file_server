#include "async_logger.h"
#include <pthread.h>

const char* async_log::level_str(Level level) noexcept {
    if (level == Level::debug) {
        return "DEBUG";
    }
    else if (level == Level::warn) {
        return "WARN";
    }
    else if (level == Level::info) {
        return "INFO";
    }
    else if (level == Level::error) {
        return "ERROR";
    }
    else {
        return "FATAL";
    }
}

void async_log::LogQueue::push(const std::string& msg) {
    std::lock_guard<std::mutex> guard{ mut };
    messages.push(msg);
    cv.notify_one();
}

bool async_log::LogQueue::pop(std::string& outMsg) {
    std::unique_lock<std::mutex> ulock{ mut };

    while (messages.empty() && running) {
        cv.wait(ulock);
    }

    if (!running && messages.empty()) {
        return false;
    }

    outMsg = std::move(messages.front());
    messages.pop();
    return true;
}

void async_log::LogQueue::shutdown() {
    std::lock_guard<std::mutex> guard{ mut };
    running = false;
    cv.notify_all();
}

async_log::Logger::Logger() {
    workThread = std::thread{ &Logger::process_queue, this };
}

void async_log::Logger::process_queue() {
    std::string msg;
    while (logQueue.pop(msg)) {
        stream << msg;
        stream.flush();
    }
}

void async_log::Logger::log(const char* fileName, const char* funcName, int lineNumber, Level lev, const std::string& msg) {
    std::string tmp;
    tmp.reserve(128);

    tmp += "[";
    tmp += utils::get_current_time();
    tmp += "] ";
    tmp += fileName;
    tmp += " ";
    tmp += funcName;
    tmp += "(";
    tmp += std::to_string(lineNumber);
    tmp += ") tid: ";
    tmp += std::to_string(pthread_self());
    tmp += " [";
    tmp += level_str(lev);
    tmp += "] ";
    tmp += msg;

    logQueue.push(tmp);
}

bool async_log::Logger::check_log_level(Level tmp) noexcept {
    return static_cast<int>(tmp) >= static_cast<int>(level);
}

void async_log::Logger::open_file(const std::string& logPath) {
    stream.open(logPath, std::ios::app);
}

async_log::Logger::~Logger() {
    logQueue.shutdown();

    if (workThread.joinable()) {
        workThread.join();
    }

    stream.flush();
}
