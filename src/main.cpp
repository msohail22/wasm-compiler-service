#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdlib>
#include <filesystem>
#include <iostream>

#include "db.h"
#include "http_server.h"

namespace fs = std::filesystem;

int main() {
    fs::path db_path = fs::current_path() / "testcases.db";
    if (!ensure_database(db_path)) {
        std::cerr << "failed to initialize database" << std::endl;
        return 1;
    }

    int port = 8080;
    const char *env_port = std::getenv("PORT");
    if (env_port) {
        port = std::atoi(env_port);
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "socket failed" << std::endl;
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port));

    if (bind(server_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) != 0) {
        std::cerr << "bind failed" << std::endl;
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 16) != 0) {
        std::cerr << "listen failed" << std::endl;
        close(server_fd);
        return 1;
    }

    std::cout << "listening on port " << port << std::endl;

    while (true) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        handle_client(client_fd, db_path);
    }

    close(server_fd);
    return 0;
}
