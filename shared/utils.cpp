#include <iostream>
#include <netinet/in.h>
#include <string>
#include <cstdint>
#include <unistd.h>
#include <crypto.h>

using namespace std;

// this is a general function to send out a message (msg) across a socket (fd)
int send_all(int fd, const string& msg)
{
    string data = msg; // we copy our message because we're encrypting in-place, which we can't do on a const parameter
    size_t bytesSent{};
    uint32_t len = htonl(msg.size()); // length of our message

    // we will FIRST send out the length of our message, so that the receiving end can properly store it
    int sent = send(fd, &len, sizeof(len), 0);

    if (sent == -1)
    {
        cout << "Problem Sending." << endl;
        return -1;
    }

    // we encrypt our sent message first!
    encryptInPlace(data);

    // this will loop as long as however much we sent isn't equal to what we're supposed to send
    while (bytesSent < data.size())
    {
        // we're sending the data whilst making sure we're moving sequentially across our buffer
        // so that we don't send duplicates/stay in place
        sent = send(fd, data.data() + bytesSent, data.size() - bytesSent, 0);

        if (sent == -1)
        {
            cout << "Problem Sending." << endl;
            return -1;
        }

        bytesSent += sent;
    }
    
    return bytesSent; // return however much we sent
}

// this is a general function to receive data from a socket (fd)
string recv_all(int fd)
{
    uint32_t len{};
    int rd = read(fd, &len, 4); // we read however long our message is going to be
    len = ntohl(len); // the length we read must be converted to host-readable bytes

    if (rd == -1)
    {
        return ""; // if we error out, we return an empty string to whoever is receiving
    }

    size_t bytesRead{}; // this will keep track of how much we read
    string str(len, '\0'); // this will store what we read -- notice it's the exact length of what we'll read and delimited well

    while (bytesRead < len)
    {
        // we're receiving the data whilst making sure we're moving sequentially across our buffer
        // so that we don't store duplicates/stay in place
        int read = recv(fd, str.data()+bytesRead, len-bytesRead, 0);

        if (read == -1)
        {
            return "";
        }
        else if (read == 0)
        {
            cout << "Session Exited." << endl;
            break;
        }

        bytesRead += read; // incrementing by however much we read
    }

    encryptInPlace(str); // it's XOR, so running in-place encryption on already-encrypted text will decrypt!

    return str; // returning the string we read to whoever's receiving
}