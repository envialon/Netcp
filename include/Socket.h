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

    sockaddr_in remote_address_;
    int fd_;
    sockaddr_in recv_address_;

public:
    Socket(const sockaddr_in& address);
    ~Socket();

    sockaddr_in GetRecvAdress();

    int SendTo(const void* map_pointer, size_t map_length, const sockaddr_in& address);
    int ReceiveFrom(void* map_pointer, size_t map_length);
    Message RecieveMessage();
    void PrintMessage(Message& message);

};


sockaddr_in make_ip_address(int port = 0, const std::string& ip_address = "0.0.0.0");