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

using namespace std;

atomic<bool> running = false;
int sockfd = -1;

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
        cout << "Issue Establishing Listener." << endl;
        close(listenfd);
        return -1;
    }

    return listenfd;
}

string recv_all(int fd)
{
    uint32_t len{};
    int rd = read(fd, &len, 4);
    len = ntohl(len);

    if (rd == -1)
    {
        return "";
    }

    size_t bytesRead{};
    string str(len, '\0');

    while (bytesRead < len)
    {
        int reading = recv(fd, str.data()+bytesRead, len-bytesRead, 0);
        if (reading == -1)
        {
            return "";
        }
        else if (reading == 0)
        {
            cout << "Session Exited." << endl;
            break;
        }
        bytesRead += reading;
    }

    return str;
}

int accepting(int fd, atomic<bool>& running)
{
    sockaddr_in clientAddr;
    socklen_t len = sizeof(clientAddr);
    unordered_set<string> seenIPs;

    while (running)
    {   
        int connfd = accept(fd, reinterpret_cast<sockaddr*>(&clientAddr), &len);
        
        if (connfd == -1)
        {
            cout << "Connection Error. Trying Again." << endl;
            this_thread::sleep_for(chrono::seconds(2));
            continue; // should we try again?
        }

        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, ipStr, INET_ADDRSTRLEN);

        if (seenIPs.find(ipStr) == seenIPs.end())
        {
            cout << "New connection: " << ipStr << endl;
            seenIPs.insert(ipStr);
        }

        string output = recv_all(connfd);

        if (output.empty())
        {
            cout << "Empty String." << endl; // possible error
            close(connfd);
            continue;
        }

        cout << output << endl;
        close(connfd);
    }

    return 0;
}


int main()
{
    running = true;
    int port = 6666;
    sockfd = server_setup(6666);

    signal(SIGINT, closeSocket);
    signal(SIGTERM, closeSocket);

    thread acceptThread(accepting, sockfd, ref(running));

    acceptThread.join(); // wait here
}