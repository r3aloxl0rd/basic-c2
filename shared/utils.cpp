#include <iostream>
#include <netinet/in.h>
#include <string>
#include <cstdint>
#include <unistd.h>
#include <crypto.h>

using namespace std;

int send_all(int fd, const string& msg)
{
    string data = msg;
    size_t bytesSent{};
    uint32_t len = htonl(msg.size());

    int sent = send(fd, &len, sizeof(len), 0);

    if (sent == -1)
    {
        cout << "Problem Sending." << endl;
        return -1;
    }

    encryptInPlace(data);

    while (bytesSent < data.size())
    {
        sent = send(fd, data.data() + bytesSent, data.size() - bytesSent, 0);

        if (sent == -1)
        {
            cout << "Problem Sending." << endl;
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
            cout << "Session Exited." << endl;
            break;
        }
        bytesRead += reading;
    }

    encryptInPlace(str);

    return str;
}