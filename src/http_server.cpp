#include "http_server.h"

#include "execute_handler.h"
#include "json_utils.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <sstream>
#include <string>

static bool send_all(int fd, const std::string &data) {
    size_t total = 0;
    while (total < data.size()) {
        ssize_t sent = send(fd, data.data() + total, data.size() - total, 0);
        if (sent <= 0) {
            return false;
        }
        total += static_cast<size_t>(sent);
    }
    return true;
}

void handle_client(int client_fd, const fs::path &db_path) {
    std::string request;
    char buffer[4096];
    ssize_t n = 0;
    while ((n = recv(client_fd, buffer, sizeof(buffer), 0)) > 0) {
        request.append(buffer, buffer + n);
        if (request.find("\r\n\r\n") != std::string::npos) {
            break;
        }
    }

    size_t header_end = request.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        close(client_fd);
        return;
    }

    std::string header = request.substr(0, header_end);
    std::string body = request.substr(header_end + 4);

    std::istringstream header_stream(header);
    std::string request_line;
    std::getline(header_stream, request_line);
    request_line = trim_left(request_line);

    if (request_line.rfind("POST /execute", 0) != 0) {
        std::string resp = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        send_all(client_fd, resp);
        close(client_fd);
        return;
    }

    size_t content_length = 0;
    std::string line;
    while (std::getline(header_stream, line)) {
        auto pos = line.find(':');
        if (pos == std::string::npos) {
            continue;
        }
        std::string key = line.substr(0, pos);
        std::string value = trim_left(line.substr(pos + 1));
        if (key == "Content-Length") {
            content_length = static_cast<size_t>(std::stoul(value));
        }
    }

    while (body.size() < content_length) {
        n = recv(client_fd, buffer, sizeof(buffer), 0);
        if (n <= 0) {
            break;
        }
        body.append(buffer, buffer + n);
    }

    std::string response_body = handle_execute(body, db_path);
    std::ostringstream resp;
    resp << "HTTP/1.1 200 OK\r\n";
    resp << "Content-Type: application/json\r\n";
    resp << "Content-Length: " << response_body.size() << "\r\n";
    resp << "Connection: close\r\n\r\n";
    resp << response_body;

    send_all(client_fd, resp.str());
    close(client_fd);
}
