#pragma once

class MyTempFile {
public:
    MyTempFile() = default;
    bool initialise();
    ~MyTempFile();

    static void delete_file() noexcept;
    const char* get_exec_path() const noexcept  __attribute__((pure));
};