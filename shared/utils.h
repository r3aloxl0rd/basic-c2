#pragma once

#include <string>

int send_all(int fd, const std::string& msg);
std::string recv_all(int fd);