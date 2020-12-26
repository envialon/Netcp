#include "Socket.h"

sockaddr_in make_ip_address(int port, const std::string& ip_address) {
    sockaddr_in output_address{};
    output_address.sin_family = AF_INET;
    inet_aton(ip_address.c_str(), &output_address.sin_addr);
    output_address.sin_port = htons(port);

    return output_address;
}

Socket::Socket(const sockaddr_in& address) {
    fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd_ < 0) {
        throw std::system_error(errno, std::system_category(), "failed to create the socket");
    }

    int bind_result = bind(fd_, reinterpret_cast<const sockaddr*>(&address), sizeof(address));
    if (bind_result < 0) {
        throw std::system_error(errno, std::system_category(), "bind failed");
    }

}

Socket::~Socket() {
    close(fd_);
}

int Socket::send_to(const void* map_pointer, unsigned int map_length, const sockaddr_in& address) {
    remote_address_ = address;
    int send_result = sendto(fd_, map_pointer, map_length, 0,
        reinterpret_cast<const sockaddr*>(&address),
        sizeof(address));

    std::cout << "here 2\n";
    if (send_result < 0) {
        throw std::system_error(errno, std::system_category(), "sendto failed");
    }

    return send_result;
}

// int Socket::recieve_from(void* map_pointer, unsigned int map_length, sockaddr_in& address) {
//     socklen_t address_len = sizeof(address);
//     remote_address_ = address;
//     int recieve_result = recvfrom(fd_, map_pointer, map_length, 0, reinterpret_cast<sockaddr*>(&address), &address_len);
//     if (recieve_result < 0) {
//         throw std::system_error(errno, std::system_category(), "recvfrom failed");
//     }
//     return recieve_result;
// }

int Socket::recieve_from(Message& message, sockaddr_in& address) {
    socklen_t address_len = sizeof(address);
    remote_address_ = address;
    int recieve_result = recvfrom(fd_, &message, sizeof(message), 0, reinterpret_cast<sockaddr*>(&address), &address_len);
    if (recieve_result < 0) {
        throw std::system_error(errno, std::system_category(), "recvfrom failed");
    }
}
// int  Socket::recieve_message(Message& message, sockaddr_in& address) {
//     socklen_t address_len = sizeof(address);
//     remote_address_ = address;
//     int recieve_result = recvfrom(fd_, &message, sizeof(message), 0, reinterpret_cast<sockaddr*>(&address), &address_len);
//     if (recieve_result < 0) {
//         throw std::system_error(errno, std::system_category(), "recvfrom failed");
//     }
//     return recieve_result;
// }

void Socket::printMessage(Message& message) {
    char* remote_IP = inet_ntoa(remote_address_.sin_addr);
    int remote_port = ntohs(remote_address_.sin_port);
    std::cout << "El sistema " << remote_IP << ":" << remote_port <<
        " envió el mensaje ' " << message.text.data() << "'\n";
}