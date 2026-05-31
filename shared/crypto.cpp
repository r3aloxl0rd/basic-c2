#include <string>

using namespace std;

static const char XOR_KEY = 0x5A;

// basic XOR encryption working with a hardcoded key -- this is just for PoC
void encryptInPlace(string& data)
{
    for (auto& element : data)
    {
        element ^= XOR_KEY;
    }
}