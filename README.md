# c2-basic — Minimal TCP Client/Server in C++

A minimal pull-based TCP client/server implementation built from scratch in C++. Built as a systems programming portfolio project, targeting Linux.

## Rationale

### Pull-based vs. Push-based
Pull-based means that the server does not initiate a connection to the deployed agents; it merely receives a connection. This is usually more prevalent in real-world attack scenarios where establishing a consistent TCP connection in order to push commands is noisy and easy to spot.
### Linux
Most servers world-wide utilize a Linux/GNU distribution as their OS. So, from an attacker perspective, you have the most ROI if your C2 can be deployed on Linux machines. Not to mention, most attackers use a Linux distribution on their machine to start. Nonetheless, there's not much modification needed to make this work for Windows machines as well.
### C++
The primary reason I like C++ is because it's a powerful low-level language that isn't as tedious or archaic to deal with as C. That said, this project could be completed in virtually any language, and can probably be coded safer more easily in a language like Rust.

## Concepts Demonstrated
- BSD socket API (socket, bind, listen, accept, connect)
- POSIX multithreading with std::thread
- Thread-safe shared state with std::mutex and lock_guard
- Length-prefixed TCP framing to handle message boundaries
- Process execution and output capture via popen()
- XOR stream cipher for transport obfuscation
- RAII resource management
- CMake multi-binary build system

## Build

```bash
mkdir build && cd build
cmake ..
make
```

## Usage

First, you start the server with:
```bash
./server
```
This ensures that the server is listening on a given port. It'll accept any incoming connection (keep in mind this is a demo tool; so this won't work over the Internet).

Then start the client:
```bash
./client
```
Theoretically, this client would be deployed on a target machine. It'll need to be running on that machine in order for the server to catch its connection attempt, and for it to execute any queued commands.

## Stack

- C++17 (There isn't anything offered in later versions of C++ that would've greatly aided this minimal project.)
- CMake
- BSD socket API
- Linux

# DISCLAIMER
This tool is meant strictly for educational and demonstrative purposes.
