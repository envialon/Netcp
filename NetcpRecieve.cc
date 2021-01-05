#include "Socket.h"
#include "File.h"
#include <exception>

int protected_main(int argc, char* argv[]) {
    sockaddr_in recieveSocketAdress(make_ip_address(6660, "127.0.0.1"));
    Socket recieveSocket(recieveSocketAdress);
    Message messageToReceive{};

    recieveSocket.receive_message(messageToReceive, recieveSocketAdress);
    File output(&messageToReceive.text[0], messageToReceive.file_size);
    while (true) {

    }
    return 0;
}

int main(int argc, char* argv[]) {

    try {
        return protected_main(argc, argv);
    }
    catch (std::system_error& e) {
        std::cerr << "NetcpRecieve: " << e.what() << '\n';
        return 2;
    }
    catch (...) {
        std::cout << "Unknown error \n";
        return 99;
    }
}