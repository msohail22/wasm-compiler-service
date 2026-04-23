#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <unistd.h>
#include <netinet/in.h>

#define PORT 8080
#define BUFFER_SIZE 8192

std::string extract_body(const std::string& req) {
    size_t pos = req.find("\r\n\r\n");
    if (pos == std::string::npos) return "";
    return req.substr(pos + 4);
}

std::string compile_to_wasm(const std::string& code) {
    std::string id = std::to_string(rand());

    std::string src = "/tmp/" + id + ".c";
    std::string out = "/tmp/" + id + ".wasm";

    // write source
    std::ofstream ofs(src);
    ofs << code;
    ofs.close();

    // compile
    std::string cmd =
        "clang --target=wasm32 -O2 -nostdlib "
        "-Wl,--no-entry -Wl,--export-all "
        "-o " + out + " " + src;

    int res = system(cmd.c_str());
    if (res != 0) {
        throw std::runtime_error("Compilation failed");
    }

    // read wasm
    std::ifstream ifs(out, std::ios::binary);
    std::stringstream buffer;
    buffer << ifs.rdbuf();

    return buffer.str();
}

void handle_client(int client_fd) {
    char buffer[BUFFER_SIZE];
    int bytes = read(client_fd, buffer, BUFFER_SIZE - 1);

    if (bytes <= 0) {
        close(client_fd);
        return;
    }

    buffer[bytes] = '\0';
    std::string request(buffer);

    std::string body = extract_body(request);

    try {
        std::string wasm = compile_to_wasm(body);

        std::string header =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/octet-stream\r\n"
            "Content-Length: " + std::to_string(wasm.size()) + "\r\n\r\n";

        send(client_fd, header.c_str(), header.size(), 0);
        send(client_fd, wasm.data(), wasm.size(), 0);

    } catch (const std::exception& e) {
        std::string err = e.what();

        std::string header =
            "HTTP/1.1 500 Internal Server Error\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: " + std::to_string(err.size()) + "\r\n\r\n";

        send(client_fd, header.c_str(), header.size(), 0);
        send(client_fd, err.c_str(), err.size(), 0);
    }

    close(client_fd);
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    bind(server_fd, (sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 10);

    std::cout << "Listening on port " << PORT << std::endl;

    while (true) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        handle_client(client_fd);
    }

    return 0;
}