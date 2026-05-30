#include <string>

using namespace std;

static const char XOR_KEY = 0x5A;

void encryptInPlace(string& data)
{
    for (auto& element : data)
    {
        element ^= XOR_KEY;
    }
}