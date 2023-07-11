#pragma once

#include <spdlog/spdlog.h>
#include <memory>
#include "accurateTime.h"
#include "AppException.h"

extern std::unique_ptr<spdlog::logger> mainlog;

void initMainLog();

class TraceHelper {
public:
    explicit TraceHelper(const char *label) : _label(label) {
        mainlog->trace("Entering {}", label);
        _start = accurateTime();
    }

    ~TraceHelper() {
        end();
    }

    void end() {
        if (_label) {
            mainlog->trace("Leaving {} [{:.6f}s]", _label, accurateTime() - _start);
            _label = nullptr;
        }
    }

private:
    const char *_label;
    double _start;
};

#define SPDLOG_TRACE_FUNC \
    TraceHelper _fth(__FUNCTION__)

#define runtime_check(expr, msg, ...) \
    if (!(expr)) {                  \
        auto s = fmt::format(msg, __VA_ARGS__); \
        throw AppException(s); \
    }
