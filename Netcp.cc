#include <exception>
#include <thread>
#include <vector>
#include <atomic>
#include <sstream>
#include "File.h"
#include "Socket.h"

#define MAX_PACKAGE_SIZE 65528

void NetcpSend(std::exception_ptr& eptr, std::string& filename) {
    try {
        Socket sendSocket(make_ip_address(0, "127.0.0.1"));
        File input(filename.c_str());
        Message messageToSend{};
        messageToSend.text[1023] = '\0';
        sockaddr_in remoteSocket = make_ip_address(6660, "127.0.0.1");

        // enviamos la información previa al archivo mapeado
        messageToSend = input.GetMapInfo(filename);
        sendSocket.send_to(&messageToSend, sizeof(messageToSend), remoteSocket);

        std::cout << "here\n";
        // enviamos el archivo mapeado
        char* aux_pointer = (char*)input.GetMapPointer();
        int aux_length = input.GetMapLength();

        if (aux_length > MAX_PACKAGE_SIZE) {
            sendSocket.send_to(aux_pointer, aux_length, remoteSocket);
        }
        else {
            char* whileThreshold = (char*)(input.GetMapPointer()) + (input.GetMapLength());
            while (aux_pointer <= whileThreshold) {
                aux_length =
                    sendSocket.send_to(aux_pointer, MAX_PACKAGE_SIZE, remoteSocket);
                aux_pointer += aux_length;
            }
        }
    }
    catch (...) {
        eptr = std::current_exception();
    }
}

void NetcpRecieve(std::exception_ptr& eptr, std::string& pathname, std::atomic_bool& pause, std::atomic_bool& abortRecieve) {

    sockaddr_in recieveSocketAdress(make_ip_address(6660, "127.0.0.1"));
    Socket recieveSocket(recieveSocketAdress);
    Message messageToRecieve{};


    // int mkdir_result = mkdir(pathname.c_str(), S_IRWXU);

    // if (mkdir_result < 0) {
    //     throw std::system_error(errno, std::system_category(), "mkdir failed");
    // }
    recieveSocket.recieve_from(&messageToRecieve, sizeof(messageToRecieve), recieveSocketAdress);
    File output(&messageToRecieve.text[0], messageToRecieve.file_size);


    char* auxptr = (char*)output.GetMapPointer();
    const char* outputThreshold = (char*)(output.GetMapPointer()) + (output.GetMapLength());
    //wait for all the packages and store them in the mapped file.
    while (auxptr < outputThreshold) {
        auxptr += recieveSocket.recieve_from((void*)auxptr, output.GetMapLength(), recieveSocketAdress);
    }

    output.WriteMappedFile();


}

void askForInput() {

    std::atomic_bool exit, pause, abortSend, abortRecieve;
    std::string userInput, pathname, filename;

    std::vector<std::thread>vecOfThreads;
    try {
        while (!exit) {
            std::cout << "introduce a command:";
            std::getline(std::cin, userInput);

            std::stringstream sstream(userInput);
            sstream >> userInput;

            if (userInput == "quit") {
                exit = true;
            }
            else if (userInput == "resume") {
                pause = false;
            }
            else if (userInput == "pause") {
                pause = true;
            }
            else if (userInput == "abort") {

                sstream >> userInput;

                if (userInput == "recieve") {
                    abortRecieve = true;
                }
                else {
                    abortSend = true;
                }
            }
            else if (userInput == "send") {
                sstream >> filename;
                if (filename.empty()) {
                    std::cout << "comando incompleto: send [FilenameToSend]\n";
                }
                else {
                    std::exception_ptr eptr{};
                    std::thread sendThread(&NetcpSend, std::ref(eptr), std::ref(filename));
                    vecOfThreads.push_back(std::move(sendThread));
                    sendThread.join();
                    if (eptr) {
                        std::rethrow_exception(eptr);
                    }
                }
            }
            else if (userInput == "recieve") {
                sstream >> pathname;
                if (pathname.empty()) {
                    std::cout << "comando incompleto: recieve [PathnameToSaveFile]\n";
                }
                else {
                    std::exception_ptr eptr{};
                    std::thread recieveThread(&NetcpRecieve, std::ref(eptr), std::ref(pathname), std::ref(pause), std::ref(abortRecieve));
                    recieveThread.detach();
                    vecOfThreads.push_back(std::move(recieveThread));

                    // recieveThread.join();
                    // if (eptr) {
                    //     std::rethrow_exception(eptr);
                    // }
                }
            }

            for (long unsigned int i = 0; i < vecOfThreads.size(); i++) {
                vecOfThreads[i].join();
            }
        }
    }
    catch (std::system_error& e) {
        std::cerr << e.what() << "\n";
    }

}

int main() {
    askForInput();
    return 0;
}