#include <iostream>

#include "prog_handler.hh"

ProgHandler::ProgHandler(ProgHandler&& other) noexcept {
    child = other.child;
    other.child = std::nullopt;
}

ProgHandler& ProgHandler::operator=(ProgHandler&& other) noexcept {
    if ((child.has_value()) && (other.child.has_value())) {
        const auto& child_v = *child;
        const auto& other_child_v = *other.child;
        if (child_v.pid == other_child_v.pid) {
            // case of self-move
            other.child = std::nullopt;
            return *this;
        }
    }

    if (child.has_value()) {
        kill_child();
        const auto& child_v = *child;
        close(child_v.stdout_fd);
        close(child_v.stdin_fd);
    }

    child = other.child;
    other.child = std::nullopt;
    return *this;
}

auto ProgHandler::try_new(const char* const prog_exec) noexcept -> std::expected<ProgHandler, std::string> {

    std::array<int, 2> child_out;
    std::array<int, 2> child_in;

    const auto child_out_pipe_res = pipe2(&child_out[0], O_CLOEXEC);
    if (child_out_pipe_res != 0) {
        return std::unexpected(std::format("failed to create pipe for the output of the child. Error code is {}", child_out_pipe_res));
    }
    const auto child_in_pipe_res = pipe2(&child_in[0], O_CLOEXEC);
    if (child_in_pipe_res != 0) {
        return std::unexpected(std::format("failed to create pipe for the input of the child. Error code is {}", child_in_pipe_res));
    }

    const auto close_fds_and_get_err = [&](const char* fn_name, int ret_code) {
        close(child_in[0]);
        close(child_out[1]);
        close(child_in[1]);
        close(child_out[0]);
        auto ret_msg = std::format("Error: {} failed with return code {}: {}", fn_name, ret_code, strerror(ret_code));
        return ret_msg;
    };

    pid_t child_pid;
    posix_spawn_file_actions_t file_actions = {};
    if (const auto file_init_res = posix_spawn_file_actions_init(&file_actions);
            file_init_res != 0) {
        auto ret_msg = close_fds_and_get_err("posix_spawn_file_actions_init", file_init_res);
        return std::unexpected(std::move(ret_msg));
    }

    const auto close_ressources_and_get_err = [&](const char* fn_name, int ret_code) {
        posix_spawn_file_actions_destroy(&file_actions);
        auto ret_msg = close_fds_and_get_err(fn_name, ret_code);
        return std::unexpected(std::move(ret_msg));
    };


    if (const auto ret = posix_spawn_file_actions_addclose(&file_actions, child_in[1]); ret != 0) {
        auto ret_msg = close_ressources_and_get_err("posix_spawn_file_actions_addclose", ret);
        return std::unexpected(std::move(ret_msg));
    }
    if (const auto ret = posix_spawn_file_actions_adddup2(&file_actions, child_in[0], STDIN_FILENO); ret != 0) {
        auto ret_msg = close_ressources_and_get_err("posix_spawn_file_actions_adddup2", ret);
        return std::unexpected(std::move(ret_msg));
    }
    if (const auto ret = posix_spawn_file_actions_addclose(&file_actions, child_in[0]); ret != 0) {
        auto ret_msg = close_ressources_and_get_err("posix_spawn_file_actions_addclose", ret);
        return std::unexpected(std::move(ret_msg));
    }

    if (const auto ret = posix_spawn_file_actions_addclose(&file_actions, child_out[0]); ret != 0) {
        auto ret_msg = close_ressources_and_get_err("posix_spawn_file_actions_addclose", ret);
        return std::unexpected(std::move(ret_msg));
    }
    if (const auto ret = posix_spawn_file_actions_adddup2(&file_actions, child_out[1], STDOUT_FILENO); ret != 0) {
        auto ret_msg = close_ressources_and_get_err("posix_spawn_file_actions_adddup2", ret);
        return std::unexpected(std::move(ret_msg));
    }
    if (const auto ret = posix_spawn_file_actions_addclose(&file_actions, child_out[1]); ret != 0) {
        auto ret_msg = close_ressources_and_get_err("posix_spawn_file_actions_addclose", ret);
        return std::unexpected(std::move(ret_msg));
    }


    const std::array<char*, 1> child_argv = { nullptr };
    const std::array<char*, 1> child_env = { nullptr };

    const auto spawn_ret = posix_spawn(&child_pid,
                                       prog_exec,
                                       &file_actions,
                                       nullptr,
                                       &child_argv[0],
                                       &child_env[0]);
    if (spawn_ret != 0) {
        auto ret_err = close_ressources_and_get_err("posix_spawn", spawn_ret);
        return std::unexpected(std::move(ret_err));
    }

    const auto child_out_fd = child_out[0];

    posix_spawn_file_actions_destroy(&file_actions);
    close(child_in[0]);
    close(child_out[1]);

    auto res = ProgHandler(child_data_t{.pid = child_pid, .stdin_fd = child_in[1], .stdout_fd = child_out_fd});
    return res;
}

ProgHandler::~ProgHandler() noexcept {
    kill_child();
}

auto ProgHandler::send_to_child(const std::string& msg, std::chrono::milliseconds timeout) const -> bool {
    if (!child.has_value()) {
        return false;
    }

    constexpr const auto max_sleep_at_a_time = std::chrono::milliseconds(5);
    std::chrono::milliseconds total_slept {0};

    const auto& child_data = child.value();
    const auto child_in_fd = child_data.stdin_fd;
    auto* msg_ptr = msg.data();
    const auto nr_bytes = msg.size();

    auto nr_bytes_left_to_write = nr_bytes;
    while (nr_bytes_left_to_write > 0) {
        errno = 0;
        ssize_t write_ret;
        do {
            write_ret = write(child_in_fd, msg_ptr, nr_bytes_left_to_write);
            if (write_ret > 0) {
                nr_bytes_left_to_write -= static_cast<size_t>(write_ret);
                msg_ptr += write_ret;
            } else if (write_ret == 0) {
                if (total_slept >= timeout) {
                    return false;
                } else {
                    const auto time_to_sleep = std::min(max_sleep_at_a_time, timeout - total_slept);
                    std::this_thread::sleep_for(std::chrono::milliseconds(time_to_sleep));
                    total_slept += time_to_sleep;
                }
            }
        } while ((write_ret >= 0) && (write_ret < static_cast<ssize_t>(nr_bytes_left_to_write)));

        if ((write_ret == static_cast<decltype(write_ret)>(-1))
             && (errno != EAGAIN)
             && (errno != EWOULDBLOCK)
             && (errno != EINTR)) {
            std::cout << std::format("Write to child failed with error {}: {}\n", errno, strerror(errno));
            return false;
        }

        if (total_slept >= timeout) {
            return false;
        } else {
            const auto time_to_sleep = std::min(max_sleep_at_a_time, timeout - total_slept);
            std::this_thread::sleep_for(std::chrono::milliseconds(time_to_sleep));
            total_slept += time_to_sleep;
        }
    }

    return true;
}

namespace {
    bool is_process_alive(const int pid) {
        int status;
        const auto ret_code = waitpid(pid, &status, WNOHANG | WUNTRACED | WCONTINUED);
        const auto is_alive = ret_code >= 0;
        return is_alive;
    }
}

void ProgHandler::kill_child_after_timeout(const std::chrono::milliseconds timeout) noexcept {
    if (child.has_value()) {
        const auto pid = child.value().pid;
        const auto max_wait_between_checks = std::chrono::milliseconds{5};
        auto waited_time = std::chrono::milliseconds {0};

        while ((is_process_alive(pid)) && (waited_time < timeout)) {
            const auto max_time_to_wait = timeout - waited_time;
            const auto time_to_wait = std::min(max_wait_between_checks, max_time_to_wait);
            std::this_thread::sleep_for(time_to_wait);
            waited_time += time_to_wait;
        }
        if (is_process_alive(pid)) {
            kill_child();
        } else {
            close(child->stdin_fd);
            close(child->stdout_fd);
            child = std::nullopt;
        }
    }
}

void ProgHandler::kill_child() noexcept {
    if (child.has_value()) {
        const auto pid = child.value().pid;
        if (const auto kill_ret = kill(pid, 9);
                kill_ret == -1) {
            std::cout << std::format("Failed to kill child with pid [{}]: Err: {}\n", pid, strerror(errno));
        } else {
            int wstatus;
            if (const auto waitpid_ret = waitpid(pid, &wstatus, WUNTRACED | WCONTINUED);
                    waitpid_ret == -1) {
                std::cout << std::format("waitpid returned an error: {}\n", strerror(errno));
            }
//                else {
//                    if (WIFEXITED(wstatus)) {
//                        printf("exited, status=%d\n", WEXITSTATUS(wstatus));
//                    } else if (WIFSIGNALED(wstatus)) {
//                        printf("killed by signal %d\n", WTERMSIG(wstatus));
//                    } else if (WIFSTOPPED(wstatus)) {
//                        printf("stopped by signal %d\n", WSTOPSIG(wstatus));
//                    } else if (WIFCONTINUED(wstatus)) {
//                        printf("continued\n");
//                    }
//                }
        }
        close(child->stdin_fd);
        close(child->stdout_fd);
        child = std::nullopt;
    }
}

ProgHandler::ProgHandler(child_data_t child_data) noexcept
    : child(std::move(child_data))
{
}

sig_atomic_t is_sigpipe_received = 0;
namespace {
    void on_sigpipe(int) {
        is_sigpipe_received = 1;
    }
}

bool set_sigpipe_signal_handler() {
    struct sigaction new_handler;
    new_handler.sa_handler = &on_sigpipe;
    sigemptyset(&new_handler.sa_mask);
    new_handler.sa_flags = 0;
    new_handler.sa_restorer = nullptr ;
    if (const auto sigaction_ret = sigaction(SIGPIPE, &new_handler, nullptr) ; sigaction_ret != 0) {
        std::cout << std::format("Failed to masked SIGPIPE. Err is {}\n", strerror(errno));
        return false;
    }
    return true;
}

