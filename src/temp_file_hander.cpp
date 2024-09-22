#include <stdlib.h>
#include <cstring>
#include <optional>
#include <iostream>
#include <thread>
#include <format>
#include <atomic>
#include <filesystem>
#include <array>
#include <type_traits>


#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>

#include "temp_file_handler.hh"

namespace {
#include "local_jira_server_bin.h"
}

namespace {
    static char* tmp_file_path;
    static std::atomic<bool> is_initialised;

    void delete_file_no_free_no_check() noexcept {
        // path shouldn't be null at this point
        const auto local_path = tmp_file_path;
        if (local_path == nullptr) {
            // hmm, something is wrong, but we can't do anything.
            std::cout << "can't delete file. path is nullptr\n";
            return;
        }
        std::cout << "deleting file " << local_path << "\n";
        std::remove(local_path);
    }

    void delete_file_no_free() noexcept {
        std::cout << "delete no free called\n";
        const auto was_initialised = is_initialised.exchange(false);
        if (!was_initialised) {
            std::cout << "can't delete temporary file. Wasn't initialised\n";
            return;
        }
        // race condition here. If the program gets killed while the main thread passed
        // the was_initialised check but before cleaning the file, the at_exit won't
        // pass the was_initialised check and the file won't be cleaned up.
        delete_file_no_free_no_check();
    }

    void delete_file_on_signal(int i) noexcept {
        std::quick_exit(128 + i);
    }

    bool set_cleaning_functions() {
        std::at_quick_exit(delete_file_no_free);

        bool ret = true;
        struct sigaction new_handler;
        new_handler.sa_handler = &delete_file_on_signal;
        sigemptyset(&new_handler.sa_mask);
        new_handler.sa_flags = 0;
        new_handler.sa_restorer = nullptr ;
        for (const auto signal_num : { SIGPIPE, SIGTERM, SIGINT } ) {
            if (const auto sigaction_ret = sigaction(signal_num, &new_handler, nullptr) ; sigaction_ret != 0) {
                std::cout << std::format("Failed to masked SIGPIPE. Err is {}\n", strerror(errno));
                ret = false;
            }
        }
        return ret;
    }

    bool is_would_block(int ret_code) {
        if constexpr (EAGAIN != EWOULDBLOCK) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wlogical-op"
            return (ret_code == EAGAIN) || (ret_code == EWOULDBLOCK);
#pragma GCC diagnostic pop
        } else {
            return ret_code == EAGAIN;
        }
    }

    bool write_temp_file(int fd, const std::chrono::milliseconds max_wait, const char* const data, const size_t data_size) noexcept {
        const auto max_duration_between_wait = std::chrono::milliseconds{5};
        auto waited_time = std::chrono::milliseconds{0};

        const auto nr_bytes_to_write = data_size;
        size_t total_written_bytes = 0;
        while ((total_written_bytes < nr_bytes_to_write) && (waited_time < max_wait)) {
            const auto nr_written_bytes = write(fd, data + total_written_bytes, data_size);
            if (nr_written_bytes == -1) {
                const auto cur_errno = errno;
                if (cur_errno == EINTR) {
                    // nothing special to do, just try again.
                } else if (is_would_block(cur_errno)) {
                    // nothing special to do, just try again. Sleep a bit to avoid a busy loop
                    const auto max_remaining_time = max_wait - waited_time;
                    const auto time_to_wait = std::min(max_remaining_time, waited_time);
                    std::this_thread::sleep_for(time_to_wait);
                    waited_time += time_to_wait;
                } else {
                    std::cout << std::format("Failed to write to temporary file. Err is {}: {}\n", cur_errno, strerror(cur_errno));
                    return false;
                }
            } else if (nr_written_bytes == 0) {
                // 0 bytes written and no error? something weird. Let's wait a bit to avoid a potential busy loop
                const auto max_remaining_time = max_wait - waited_time;
                const auto time_to_wait = std::min(max_remaining_time, waited_time);
                std::this_thread::sleep_for(time_to_wait);
                waited_time += time_to_wait;
            } else {
                total_written_bytes += static_cast<size_t>(nr_written_bytes);
            }
        }

        if (waited_time >= max_wait) {
            return false;
        }

        return true;
    }

    bool write_temp_file(int fd, const std::chrono::milliseconds max_wait) noexcept {
        const auto data_start = static_cast<char*>(static_cast<void*>(&__local_jira_server[0]));
        const auto data_size = __local_jira_server_len;
        if (data_size <= 0) {
            return false;
        }

        const auto ret = write_temp_file(fd, max_wait, data_start, static_cast<size_t>(data_size));
        return ret;
    }

    bool set_exec(int fd) {
        const auto f_chmod_ret = fchmod(fd, S_IXUSR);
        const auto f_chmod_errno = errno;
        const auto close_ret = close(fd);
        const auto close_errno = errno;

        if (f_chmod_ret == -1) {
            std::cout << std::format("Failed to chmod temporary file. Err is {}: {}\n", f_chmod_errno, strerror(f_chmod_errno));
            return false;
        }

        if (close_ret == -1) {
            std::cout << std::format("Failed to close temporary file. Err is {}: {}\n", close_errno, strerror(close_errno));
            return false;
        }

        return true;
    }

}

const char* MyTempFile::get_exec_path() const noexcept {
  return tmp_file_path;
}

bool MyTempFile::initialise() {
    try {
        // can't rely on mutex since we need to clean on at_exit and there would be a deadlock
        // if the program gets killed while owning the lock.
        // So instead, there is a race condition. Fine in this case since this class is used in
        // only one place, by a single thread.
        const auto tmp_dir = std::filesystem::temp_directory_path();
        const auto tmp_file_template = tmp_dir / "local_jira_server_bin.XXXXXX";
        const auto tmp_template = tmp_file_template.string();

        const auto was_initialised = is_initialised.exchange(true);
        if (was_initialised) {
            std::cout << "Error: temporary file is already set. This class is meant to be used for only one file\n";
            return false;
        }

        const auto len_str = tmp_template.size();
        const auto len_with_nul_byte = len_str + 1;
        tmp_file_path = static_cast<char *>(std::malloc(len_with_nul_byte));
        if (tmp_file_path == nullptr) {
            std::cout << "Error: allocating\n";
            return false;
        }
        std::memcpy(tmp_file_path, tmp_template.data(), len_str);
        tmp_file_path[len_with_nul_byte - 1] = '\0';

        const auto mkf_ret = mkostemp(tmp_file_path, 0);
        if (mkf_ret == -1) {
            std::cout << std::format("Error while creating the temporary file. File template is: error code {}: {}\n",
                                     tmp_file_path, errno, strerror(errno));
            free(tmp_file_path);
            tmp_file_path = nullptr;
            is_initialised = false;
            return false;
        }
        set_cleaning_functions();
        std::cout << std::format("Temporary file will be at {}.\n", tmp_file_path);
        const auto ret_write = write_temp_file(mkf_ret, std::chrono::milliseconds{500});
        if (!ret_write) {
            free(tmp_file_path);
            tmp_file_path = nullptr;
            is_initialised = false;
            return false;
        }
        set_exec(mkf_ret);
        return true;
    } catch (const std::exception &e) {
        std::cout << std::format("Failed to initialise temporary. Err is {}\n", e.what());
        is_initialised = false;
        return false;
    } catch (...) {
        std::cout << std::format("Failed to initialise temporary.\n");
        is_initialised = false;
        return false;
    }
}

MyTempFile::~MyTempFile() { delete_file(); }

void MyTempFile::delete_file() noexcept {
    const auto was_initialised = is_initialised.exchange(false);
    if (!was_initialised) {
        return;
    }
    delete_file_no_free_no_check();
    if (tmp_file_path != nullptr) {
        // hmm, something is wrong, but we can't do anything.
        free(tmp_file_path);
        tmp_file_path = nullptr;
    }
}

