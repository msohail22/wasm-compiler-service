#ifndef EXEC_SERVICE_EXECUTE_HANDLER_H
#define EXEC_SERVICE_EXECUTE_HANDLER_H

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

std::string handle_execute(const std::string &body, const fs::path &db_path);

#endif
