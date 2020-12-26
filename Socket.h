#include <sys/types.h>
#include <netinet/ip.h>
#include <string.h>
#include <iostream>
#include <arpa/inet.h>
#include <array>
#include <unistd.h>
#include "Message.h"

class Socket {
private:
    int fd_;
    sockaddr_in remote_address_;

public:
    Socket(const sockaddr_in& address);
    ~Socket();

    int send_to(const void* map_pointer, unsigned int map_length, const sockaddr_in& address);
    int recieve_from(void* map_pointer, unsigned int map_length, sockaddr_in& address);
    int recieve_message(Message& message, sockaddr_in& address);
    void printMessage(Message& message);

};


sockaddr_in make_ip_address(int port = 0, const std::string& ip_address = "0.0.0.0");