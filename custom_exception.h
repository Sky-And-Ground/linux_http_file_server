#pragma once

#include <stdexcept>
#include <system_error>

class Exception : public std::runtime_error {
public:
    const char* file;
    const char* func;
    int line;

    Exception(const char* _file, const char* _func, int _line, const std::string& msg)
        : std::runtime_error{ msg }, file{ _file }, func{ _func }, line{ _line }
    {}
};

class SystemError : public Exception {
public:
    std::error_code ec;

    SystemError(const char* file, const char* func, int line, const std::error_code& _ec, const std::string& msg)
        : Exception{ file, func, line, msg }, ec{ _ec }
    {}
};

#define THROW_EXCEPTION(msg)         throw Exception{ __FILE__, __func__, __LINE__, msg }

#define THROW_SYSTEM_ERROR(ec, msg)  throw SystemError{ __FILE__, __func__, __LINE__, ec, msg }