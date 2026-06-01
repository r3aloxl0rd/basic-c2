#pragma once

#include <atomic>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_set>
#include <vector>

using namespace std;

// this holds the variables our server needs to function
struct ServerState {
    int sockfd = -1;
    std::atomic<bool> running = true;
    queue<string> commands{};
    unordered_set<std::string> seenIPs;
    mutex deadlock;
    vector<string> history;
};

extern ServerState server;

int connection_handler(int fd, std::atomic<bool>& running);
int server_setup(int port);
