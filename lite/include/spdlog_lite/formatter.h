// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include <chrono>
#include <ctime>
#include <format>
#include <string>

#include "details/log_msg.h"

namespace spdlog_lite {

// Simple fixed-format formatter.
// Output: [2024-01-15 10:30:45.123] [logger_name] [info] message\n
// Uses hand-rolled timestamp formatting for performance (std::format with chrono is very slow on MSVC).
struct simple_formatter {

    static void pad2(int n, std::string &dest) {
        dest.push_back(static_cast<char>('0' + n / 10));
        dest.push_back(static_cast<char>('0' + n % 10));
    }

    static void pad3(int n, std::string &dest) {
        dest.push_back(static_cast<char>('0' + n / 100));
        dest.push_back(static_cast<char>('0' + (n / 10) % 10));
        dest.push_back(static_cast<char>('0' + n % 10));
    }

    static void pad4(int n, std::string &dest) {
        dest.push_back(static_cast<char>('0' + n / 1000));
        dest.push_back(static_cast<char>('0' + (n / 100) % 10));
        dest.push_back(static_cast<char>('0' + (n / 10) % 10));
        dest.push_back(static_cast<char>('0' + n % 10));
    }

    void format(const details::log_msg &msg, std::string &dest) {
        using namespace std::chrono;

        auto time_since_epoch = msg.time.time_since_epoch();
        auto secs = duration_cast<seconds>(time_since_epoch);
        auto millis = duration_cast<milliseconds>(time_since_epoch) - duration_cast<milliseconds>(secs);
        auto time_t_val = static_cast<std::time_t>(secs.count());

        std::tm tm_val{};
#ifdef _WIN32
        localtime_s(&tm_val, &time_t_val);
#else
        localtime_r(&time_t_val, &tm_val);
#endif

        // [YYYY-MM-DD HH:MM:SS.mmm]
        dest.push_back('[');
        pad4(tm_val.tm_year + 1900, dest);
        dest.push_back('-');
        pad2(tm_val.tm_mon + 1, dest);
        dest.push_back('-');
        pad2(tm_val.tm_mday, dest);
        dest.push_back(' ');
        pad2(tm_val.tm_hour, dest);
        dest.push_back(':');
        pad2(tm_val.tm_min, dest);
        dest.push_back(':');
        pad2(tm_val.tm_sec, dest);
        dest.push_back('.');
        pad3(static_cast<int>(millis.count()), dest);
        dest.append("] [");
        dest.append(msg.logger_name);
        dest.append("] [");
        dest.append(to_string_view(msg.log_level));
        dest.append("] ");
        dest.append(msg.payload);
        dest.push_back('\n');
    }
};

}  // namespace spdlog_lite
