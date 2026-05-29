#include <cstdint>
#include <iostream>
#include <string>
#include <cstring>
#include <mutex>
#include <queue>
#include <system_error>
#include <thread>
#include <chrono>
#include <memory>
#include <cctype>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <atomic>

using namespace std;

int connect_to_server(const string& ip, int port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd == -1)
    {
        cout << "Socket creation failed." << endl;
        return -1;
    }

    sockaddr_in clientAddr;
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &clientAddr.sin_addr);

    socklen_t sizeClient = sizeof(clientAddr);

    int connAttempt = connect(sockfd, reinterpret_cast<sockaddr*>(&clientAddr), sizeClient);

    if (connAttempt == -1)
    {
        cout << "Error Connecting!" << endl;
        close(sockfd);
        return -1;
    }

    return sockfd;
}

int send_all(int fd, const string& msg)
{
    size_t bytesSent{};
    uint32_t len = htonl(msg.size());

    int sent = send(fd, &len, sizeof(len), 0);

    if (sent == -1)
    {
        cout << "Problem Sending." << endl;
        return -1;
    }

    while (bytesSent < msg.size())
    {
        sent = send(fd, msg.data() + bytesSent, msg.size() - bytesSent, 0);

        if (sent == -1)
        {
            cout << "Problem Sending." << endl;
            return -1;
        }

        bytesSent += sent;
    }
    
    return bytesSent;
}

void beaconing(const string& ip, int port, atomic<bool>& running)
{
    while (running)
    {
        string beaconMsg = "Beacon. Awaiting.";
        int fd = connect_to_server(ip, port);

        if (fd == -1)
        {
            this_thread::sleep_for(chrono::seconds(5));
            continue; // trying again
        }

        send_all(fd, beaconMsg); 
        close(fd);

        this_thread::sleep_for(chrono::seconds(5));
    }
}

int main()
{
    atomic<bool> running = true;

    string ip = "127.0.0.1";
    int port = 6666;

    thread beaconThread(beaconing, ref(ip), port, ref(running));

    beaconThread.join(); // wait here
}