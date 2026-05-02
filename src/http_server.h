#ifndef EXEC_SERVICE_HTTP_SERVER_H
#define EXEC_SERVICE_HTTP_SERVER_H

#include <filesystem>

namespace fs = std::filesystem;

void handle_client(int client_fd, const fs::path &db_path);

#endif
