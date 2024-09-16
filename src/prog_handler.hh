#pragma once

#include <cassert>
#include <string.h>
#include <cstring>
#include <cstdint>
#include <chrono>
#include <thread>
#include <optional>
#include <expected>
#include <array>
#include <spawn.h>
#include <format>

#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>

class ProgHandler final {
public:
    ProgHandler() = delete;
    ProgHandler(const ProgHandler& other) noexcept = delete;
    ProgHandler& operator=(const ProgHandler& other) noexcept = delete;

    ProgHandler(ProgHandler&& other) noexcept;
    ProgHandler& operator=(ProgHandler&& other) noexcept;

    ~ProgHandler() noexcept;

    static auto try_new(const char* const prog_exec) noexcept -> std::expected<ProgHandler, std::string>;

    auto send_to_child(const std::string& msg, std::chrono::milliseconds timeout = std::chrono::milliseconds{50}) const -> bool;

    template<typename ON_MSG_FN, typename ON_ERR_FN>
    auto start_background_message_listener(ON_MSG_FN on_message_received_fn, ON_ERR_FN on_error_fn) -> std::expected<std::jthread, int> {
        if (!child.has_value()) {
            return std::unexpected(4);
        }
        const auto child_stdout = child->stdout_fd;
        std::jthread background_thread (get_line_from_child, child_stdout, std::move(on_message_received_fn), std::move(on_error_fn));
        return background_thread;
    }

    void kill_child() noexcept;

private:
    struct child_data_t {
        pid_t pid;
        int stdin_fd;
        int stdout_fd;
    };

    std::optional<child_data_t> child = std::nullopt;

private:
    ProgHandler(child_data_t child_data) noexcept;

    template<typename ON_MSG_FN, typename ON_ERR_FN>
    static auto get_line_from_child(std::stop_token stop_token, const int child_stdout_fd, ON_MSG_FN on_message_received_fn, ON_ERR_FN on_error_fn) -> void {
        std::vector<std::uint8_t> storage;
        size_t nr_bytes_used_in_storage = 0;

        // constexpr auto MiB = [](unsigned i) { const auto res = i * 1024 * 1024 ; return res; };
        constexpr auto KiB = [](unsigned i) { const auto res = i * 1024 ; return res; };

        constexpr auto min_available_size = KiB(40);
        auto storage_size = storage.size();
        const auto nr_bytes_left_in_storage = [&](){ return storage_size - nr_bytes_used_in_storage; } ;

        while (!stop_token.stop_requested()) { // replaced with no cancel requested
            bool got_line = false;

            while ((!got_line) && (!stop_token.stop_requested())) {
                if (nr_bytes_left_in_storage() < min_available_size) {
                    storage_size += min_available_size;
                    storage.resize(storage_size);
                }

                auto *data_ptr = storage.data() + nr_bytes_used_in_storage;
                auto read_ret = read(child_stdout_fd, data_ptr, nr_bytes_left_in_storage());
                if (read_ret == -1) {
                    if ((errno == EINTR) || (errno == EAGAIN)) {
                        // nothing special to do. Just try again
                    } else if (errno == EWOULDBLOCK) {
                        // check for cancel requested
                        // Shouldn't get here since we don't create the pipe with O_NONBLOCK
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    } else {
                        on_error_fn(std::format("failed to read from child. Err is {}: {}\n", errno, strerror(errno)));
                        return;
                    }
                } else if (read_ret == 0) {
                    // this means EOF. The child closed the pipe (might have died)
                    // todo look at how to handle this. Maybe we can restart the server, or show an error message
                    return;
                } else {
                    nr_bytes_used_in_storage += static_cast<size_t>(read_ret);
                    const auto end_it = data_ptr + read_ret;
                    const auto newline_pos = std::find(data_ptr, end_it, '\n');
                    got_line = (newline_pos != end_it);
                }
            }

            // we got at least one line, and the end might not be \n terminated.
            auto *current_str_begin = storage.data();
            auto *const end_storage = storage.data() + nr_bytes_used_in_storage;
            decltype(current_str_begin) next_endline;
            do {
                next_endline = std::find(current_str_begin, end_storage, '\n');
                if (next_endline != end_storage) {
                    assert(next_endline < end_storage);
                    auto *const next_begin_str = std::next(next_endline);
                    auto curr_str = std::string(current_str_begin, next_begin_str); // keep the '\n' in the string
                    on_message_received_fn(std::move(curr_str));
                    current_str_begin = next_begin_str;
                }
            } while (next_endline < end_storage);

            assert(next_endline == end_storage);
            assert(current_str_begin <= end_storage);

            // from current_str_begin up to end_storage, there is no '\n' left.
            // move that part to the beginning of the vector
            const auto nr_bytes_after_last_ending = end_storage - current_str_begin;
            assert(nr_bytes_after_last_ending >= 0);
            const auto nr_bytes_after_last_ending_unsigned = static_cast<size_t>(nr_bytes_after_last_ending);
            std::memmove(storage.data(), current_str_begin, nr_bytes_after_last_ending_unsigned);
            nr_bytes_used_in_storage = nr_bytes_after_last_ending_unsigned;
        }
    }
};

extern sig_atomic_t is_sigpipe_received;
bool set_sigpipe_signal_handler();