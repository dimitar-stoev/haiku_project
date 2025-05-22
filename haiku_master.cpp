#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <arpa/inet.h>

struct MasterConfig {
    int port;
    int delay;
    std::string dictionary;
};

std::vector<MasterConfig> masters;
volatile sig_atomic_t running = 1;

void read_words(const std::string& filename, std::vector<std::string>& words) {
    std::ifstream file(filename);
    std::string word;
    while (std::getline(file, word)) {
        if (!word.empty()) words.push_back(word);
    }
    file.close();
}

void master_process(int port, int delay, const std::string& dictionary) {
    std::vector<std::string> words;
    read_words(dictionary, words);
    if (words.empty()) {
        std::cerr << "Master on port " << port << ": Empty dictionary\n";
        return;
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Master on port " << port << ": Socket creation failed\n";
        return;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Master on port " << port << ": Bind failed\n";
        close(server_fd);
        return;
    }

    if (listen(server_fd, 10) < 0) {
        std::cerr << "Master on port " << port << ": Listen failed\n";
        close(server_fd);
        return;
    }

    std::cout << "Master started on port " << port << "\n";

    std::vector<int> client_fds;
    size_t word_index = 0;

    while (running) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        int max_fd = server_fd;

        for (int fd : client_fds) {
            FD_SET(fd, &read_fds);
            if (fd > max_fd) max_fd = fd;
        }

        struct timeval timeout = {1, 0};
        int activity = select(max_fd + 1, &read_fds, nullptr, nullptr, &timeout);

        if (activity < 0 && running) {
            std::cerr << "Master on port " << port << ": Select error\n";
            continue;
        }

        if (FD_ISSET(server_fd, &read_fds)) {
            int client_fd = accept(server_fd, nullptr, nullptr);
            if (client_fd >= 0) {
                client_fds.push_back(client_fd);
                std::cout << "Master on port " << port << ": New client connected\n";
            }
        }

        if (word_index < words.size()) {
            std::string word = words[word_index] + "\n";
            for (auto it = client_fds.begin(); it != client_fds.end();) {
                if (send(*it, word.c_str(), word.size(), 0) < 0) {
                    std::cout << "Master on port " << port << ": Client disconnected\n";
                    close(*it);
                    it = client_fds.erase(it);
                } else {
                    ++it;
                }
            }
            word_index = (word_index + 1) % words.size();
            sleep(delay);
        }
    }

    for (int fd : client_fds) close(fd);
    close(server_fd);
}

void signal_handler(int sig) {
    running = 0;
}

void read_config(const std::string& config_file) {
    std::ifstream file(config_file);
    if (!file.is_open()) {
        std::cerr << "Failed to open config file: " << config_file << "\n";
        exit(EXIT_FAILURE);
    }
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        int port, delay;
        std::string dict;
        if (iss >> port >> delay >> dict) {
            masters.push_back({port, delay, dict});
        }
    }
    file.close();
}

int main(int argc, char* argv[]) {
    std::cout << "Starting haiku_master\n";
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    read_config("master_config.txt");

    if (masters.empty()) {
        std::cerr << "No masters configured in master_config.txt\n";
        return 1;
    }

    for (const auto& master : masters) {
        pid_t pid = fork();
        if (pid == 0) {
            master_process(master.port, master.delay, master.dictionary);
            exit(EXIT_SUCCESS);
        } else if (pid < 0) {
            std::cerr << "Failed to fork master on port " << master.port << "\n";
        }
    }

    while (running) {
        pause();
    }

    std::cout << "Shutting down haiku_master\n";
    return 0;
}
