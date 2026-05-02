#ifndef EXEC_SERVICE_DB_H
#define EXEC_SERVICE_DB_H

#include <filesystem>
#include <string>
#include <vector>

#include "types.h"

namespace fs = std::filesystem;

bool ensure_database(const fs::path &db_path);
std::vector<TestCase> load_test_cases(const fs::path &db_path, const std::string &question_id);

#endif
