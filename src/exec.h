#ifndef EXEC_SERVICE_EXEC_H
#define EXEC_SERVICE_EXEC_H

#include <filesystem>
#include <string>
#include <vector>

#include "types.h"

ExecResult run_process(const std::vector<std::string> &args,
                       const std::string &stdin_data,
                       int time_limit_sec,
                       size_t memory_limit_mb);

bool write_file(const std::filesystem::path &path, const std::string &content);

#endif
