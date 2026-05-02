#include "exec.h"

#include <chrono>
#include <csignal>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

#include <fstream>

ExecResult run_process(const std::vector<std::string> &args,
                       const std::string &stdin_data,
                       int time_limit_sec,
                       size_t memory_limit_mb) {
    ExecResult result;

    int in_pipe[2];
    int out_pipe[2];
    int err_pipe[2];
    if (pipe(in_pipe) != 0 || pipe(out_pipe) != 0 || pipe(err_pipe) != 0) {
        result.stderr_data = "pipe_failed";
        return result;
    }

    pid_t pid = fork();
    if (pid == 0) {
        dup2(in_pipe[0], STDIN_FILENO);
        dup2(out_pipe[1], STDOUT_FILENO);
        dup2(err_pipe[1], STDERR_FILENO);

        close(in_pipe[1]);
        close(out_pipe[0]);
        close(err_pipe[0]);

        rlimit cpu_limit{};
        cpu_limit.rlim_cur = time_limit_sec;
        cpu_limit.rlim_max = time_limit_sec + 1;
        setrlimit(RLIMIT_CPU, &cpu_limit);

        rlimit mem_limit{};
        mem_limit.rlim_cur = memory_limit_mb * 1024 * 1024;
        mem_limit.rlim_max = memory_limit_mb * 1024 * 1024;
        setrlimit(RLIMIT_AS, &mem_limit);

        std::vector<char *> argv;
        argv.reserve(args.size() + 1);
        for (const auto &arg : args) {
            argv.push_back(const_cast<char *>(arg.c_str()));
        }
        argv.push_back(nullptr);

        execvp(argv[0], argv.data());
        _exit(127);
    }

    close(in_pipe[0]);
    close(out_pipe[1]);
    close(err_pipe[1]);

    if (!stdin_data.empty()) {
        ssize_t written = write(in_pipe[1], stdin_data.data(), stdin_data.size());
        (void)written;
    }
    close(in_pipe[1]);

    auto start = std::chrono::steady_clock::now();
    bool finished = false;
    int status = 0;

    while (!finished) {
        pid_t w = waitpid(pid, &status, WNOHANG);
        if (w == pid) {
            finished = true;
            break;
        }
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
        if (elapsed > time_limit_sec * 1000LL) {
            result.timed_out = true;
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
            finished = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::string out_data;
    std::string err_data;
    char buffer[4096];
    ssize_t n = 0;
    while ((n = read(out_pipe[0], buffer, sizeof(buffer))) > 0) {
        out_data.append(buffer, buffer + n);
    }
    while ((n = read(err_pipe[0], buffer, sizeof(buffer))) > 0) {
        err_data.append(buffer, buffer + n);
    }

    close(out_pipe[0]);
    close(err_pipe[0]);

    if (!result.timed_out) {
        if (WIFSIGNALED(status)) {
            result.signaled = true;
            result.exit_code = 128 + WTERMSIG(status);
        } else if (WIFEXITED(status)) {
            result.exit_code = WEXITSTATUS(status);
        }
    }

    result.stdout_data = std::move(out_data);
    result.stderr_data = std::move(err_data);
    return result;
}

bool write_file(const std::filesystem::path &path, const std::string &content) {
    std::ofstream out(path);
    if (!out) {
        return false;
    }
    out << content;
    return true;
}
