#include <iostream>
#include <vector>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/un.h>
#include <signal.h>
#include <algorithm>

volatile sig_atomic_t running = 1;

void signal_handler(int sig) {
    running = 0;
}

void send_to_librarian(const std::string& haiku, const std::string& student_id) {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Student " << student_id << ": Failed to create UNIX socket\n";
        return;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, "/tmp/haiku_librarian", sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Student " << student_id << ": Failed to connect to librarian\n";
        close(sock);
        return;
    }

    std::string message = student_id + ":" + haiku + "\n";
    send(sock, message.c_str(), message.size(), 0);
    close(sock);
}

void connect_to_master(const std::string& host, int port, std::vector<int>& client_fds) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Student: Failed to create socket for port " << port << "\n";
        return;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Student: Failed to connect to master on port " << port << "\n";
        close(sock);
        return;
    }

    client_fds.push_back(sock);
    std::cout << "Student: Connected to master on port " << port << "\n";
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <host> <port1> <port2> ...\n";
        return 1;
    }

    std::string host = argv[1];
    std::vector<int> ports;
    for (int i = 2; i < argc; ++i) {
        ports.push_back(std::stoi(argv[i]));
    }

    std::string student_id = "student_" + std::to_string(getpid());
    signal(SIGINT, signal_handler);

    std::vector<int> client_fds;
    for (int port : ports) {
        connect_to_master(host, port, client_fds);
    }

    std::vector<std::string> haiku_lines[3];
    int line_index = 0;
    int word_counts[] = {5, 7, 5};
    int word_count = 0;

    while (running) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        int max_fd = 0;
        for (int fd : client_fds) {
            FD_SET(fd, &read_fds);
            if (fd > max_fd) max_fd = fd;
        }

        struct timeval timeout = {1, 0};
        int activity = select(max_fd + 1, &read_fds, nullptr, nullptr, &timeout);
        if (activity < 0 && running) continue;

        for (int fd : client_fds) {
            if (FD_ISSET(fd, &read_fds)) {
                char buffer[1024];
                int n = recv(fd, buffer, sizeof(buffer) - 1, 0);
                if (n <= 0) {
                    std::cerr << "Student: Master disconnected\n";
                    close(fd);
                    client_fds.erase(std::remove(client_fds.begin(), client_fds.end(), fd), client_fds.end());
                    continue;
                }

                buffer[n] = '\0';
                std::string word(buffer);
                word.erase(std::remove(word.begin(), word.end(), '\n'), word.end());
                if (word.empty()) continue;

                haiku_lines[line_index].push_back(word);
                word_count++;

                if (word_count == word_counts[line_index]) {
                    std::string line;
                    for (const auto& w : haiku_lines[line_index]) {
                        line += w + " ";
                    }
                    std::cout << line << "\n";
                    word_count = 0;
                    haiku_lines[line_index].clear();
                    line_index++;

                    if (line_index == 3) {
                        std::string haiku;
                        for (int i = 0; i < 3; ++i) {
                            for (const auto& w : haiku_lines[i]) {
                                haiku += w + " ";
                            }
                            haiku += "\n";
                        }
                        std::cout << "\n";
                        send_to_librarian(haiku, student_id);
                        line_index = 0;
                    }
                }
            }
        }
    }

    for (int fd : client_fds) close(fd);
    return 0;
}
