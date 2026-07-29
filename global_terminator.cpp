#include "global_terminator.h"
#include "async_logger.h"
#include "custom_exception.h"
#include <execinfo.h>
#include <cstdlib>

void log_stacktrace(int level) noexcept {
    void** buffer = (void**)malloc(level * sizeof(void*));
    if (buffer == NULL) {
        return;
    }

    int nptrs = backtrace(buffer, level);
    char** symbols = backtrace_symbols(buffer, nptrs);

    if (symbols == NULL) {
        free(buffer);
        return;
    }

    int i;
    for (i = 0; i < nptrs; ++i) {
        SIMPLE_LOG("%d, %s\n", i, symbols[i]);
    }

    free(symbols);
    free(buffer);
}

void global_terminate_handler() noexcept {
    auto exPtr = std::current_exception();

    if (exPtr) {
        try {
            std::rethrow_exception(exPtr);
        }
        catch (const SystemError& e) {
            LOG_ERROR("system error at %s, %s(%d), %s, value: %d, msg: %s\n", e.file, e.func, e.line, e.what(), e.ec.value(), e.ec.message().c_str());
        }
        catch (const Exception& e) {
            LOG_ERROR("runtime error at %s, %s(%d), %s\n", e.file, e.func, e.line, e.what());
        }
        catch (const std::exception& e) {
            LOG_ERROR("standard exception: %s\n", e.what());
        }
        catch (...) {
            LOG_ERROR("unknown exception\n");
        }
    }

    log_stacktrace(64);
    std::abort();
}