// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include <array>
#include <cstdio>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>

#ifdef _WIN32
#include <io.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "../details/log_msg.h"
#include "../details/null_mutex.h"
#include "../formatter.h"

namespace spdlog_lite::sinks {

namespace detail {
#ifdef _WIN32
inline void enable_ansi_colors() {
    static bool done = false;
    if (done) return;
    done = true;
    auto handle = ::GetStdHandle(STD_OUTPUT_HANDLE);
    if (handle != INVALID_HANDLE_VALUE) {
        DWORD mode = 0;
        if (::GetConsoleMode(handle, &mode)) {
            ::SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }
    }
    handle = ::GetStdHandle(STD_ERROR_HANDLE);
    if (handle != INVALID_HANDLE_VALUE) {
        DWORD mode = 0;
        if (::GetConsoleMode(handle, &mode)) {
            ::SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }
    }
}
#else
inline void enable_ansi_colors() {}
#endif
}  // namespace detail

namespace ansi_color {
constexpr std::string_view reset = "\033[m";
constexpr std::string_view white = "\033[37m";
constexpr std::string_view cyan = "\033[36m";
constexpr std::string_view green = "\033[32m";
constexpr std::string_view yellow_bold = "\033[33m\033[1m";
constexpr std::string_view red_bold = "\033[31m\033[1m";
constexpr std::string_view bold_on_red = "\033[1m\033[41m";
}  // namespace ansi_color

template <typename Mutex>
class ansicolor_sink {
public:
    explicit ansicolor_sink(std::FILE *file = stdout)
        : mutex_(std::make_unique<Mutex>()),
          file_(file) {
        detail::enable_ansi_colors();
        colors_[static_cast<std::size_t>(level::trace)] = ansi_color::white;
        colors_[static_cast<std::size_t>(level::debug)] = ansi_color::cyan;
        colors_[static_cast<std::size_t>(level::info)] = ansi_color::green;
        colors_[static_cast<std::size_t>(level::warn)] = ansi_color::yellow_bold;
        colors_[static_cast<std::size_t>(level::err)] = ansi_color::red_bold;
        colors_[static_cast<std::size_t>(level::critical)] = ansi_color::bold_on_red;
        colors_[static_cast<std::size_t>(level::off)] = ansi_color::reset;
    }

    void log(const details::log_msg &msg) {
        std::lock_guard<Mutex> lock(*mutex_);

        // Format using simple_formatter (has cached localtime)
        buf_.clear();
        formatter_.format(msg, buf_);

        // Find the level text and wrap it in color codes.
        // Format is: [timestamp] [name] [LEVEL] payload\n
        auto color = colors_[static_cast<std::size_t>(msg.log_level)];
        auto level_name = to_string_view(msg.log_level);

        std::string_view sv(buf_);
        auto pos = sv.find(level_name);
        if (pos != std::string_view::npos) {
            // Build colored output in color_buf_, single fwrite
            color_buf_.clear();
            color_buf_.append(buf_, 0, pos);
            color_buf_.append(color);
            color_buf_.append(level_name);
            color_buf_.append(ansi_color::reset);
            color_buf_.append(buf_, pos + level_name.size());
            std::fwrite(color_buf_.data(), 1, color_buf_.size(), file_);
        } else {
            std::fwrite(buf_.data(), 1, buf_.size(), file_);
        }
    }

    void flush() {
        std::lock_guard<Mutex> lock(*mutex_);
        std::fflush(file_);
    }

    void set_color(level lvl, std::string_view color) {
        colors_[static_cast<std::size_t>(lvl)] = color;
    }

private:
    std::unique_ptr<Mutex> mutex_;
    std::FILE *file_;
    simple_formatter formatter_;
    std::string buf_;
    std::string color_buf_;
    std::array<std::string_view, levels_count> colors_{};
};

struct stdout_color_sink_mt : ansicolor_sink<std::mutex> {
    stdout_color_sink_mt() : ansicolor_sink(stdout) {}
};
struct stdout_color_sink_st : ansicolor_sink<details::null_mutex> {
    stdout_color_sink_st() : ansicolor_sink(stdout) {}
};
struct stderr_color_sink_mt : ansicolor_sink<std::mutex> {
    stderr_color_sink_mt() : ansicolor_sink(stderr) {}
};
struct stderr_color_sink_st : ansicolor_sink<details::null_mutex> {
    stderr_color_sink_st() : ansicolor_sink(stderr) {}
};

}  // namespace spdlog_lite::sinks
