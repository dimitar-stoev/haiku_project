#include <iostream>
#include <fstream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <signal.h>
#include <ctime>

volatile sig_atomic_t running = 1;

void signal_handler(int sig) {
    running = 0;
}

std::string get_timestamp() {
    time_t now = time(nullptr);
    char buf[100];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    return buf;
}

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Librarian: Failed to create socket\n";
        return 1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, "/tmp/haiku_librarian", sizeof(addr.sun_path) - 1);

    unlink("/tmp/haiku_librarian");
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Librarian: Bind failed\n";
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 10) < 0) {
        std::cerr << "Librarian: Listen failed\n";
        close(server_fd);
        return 1;
    }

    std::ofstream log_file("haiku_log.txt", std::ios::app);
    if (!log_file.is_open()) {
        std::cerr << "Librarian: Failed to open log file\n";
        close(server_fd);
        return 1;
    }

    std::cout << "Librarian: Started, listening for haikus\n";

    while (running) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd < 0 && running) continue;

        char buffer[4096];
        int n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (n > 0) {
            buffer[n] = '\0';
            std::string message(buffer);
            size_t colon_pos = message.find(':');
            if (colon_pos != std::string::npos) {
                std::string student_id = message.substr(0, colon_pos);
                std::string haiku = message.substr(colon_pos + 1);
                log_file << "[" << get_timestamp() << "] " << student_id << ":\n" << haiku << "\n";
                log_file.flush();
            }
        }
        close(client_fd);
    }

    log_file.close();
    close(server_fd);
    unlink("/tmp/haiku_librarian");
    std::cout << "Librarian: Shutting down\n";
    return 0;
}
