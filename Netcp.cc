#include <exception>
#include <thread>
#include <vector>
#include <atomic>
#include <sstream>
#include "File.h"
#include "Socket.h"

#define MAX_PACKAGE_SIZE 60000

void NetcpSend(std::exception_ptr& eptr, std::string& filename) {
    try {
        Socket sendSocket(make_ip_address(0, "127.0.0.1"));
        sockaddr_in remoteSocket = make_ip_address(6660, "127.0.0.1");

        File input(filename.c_str());

        Message messageToSend{};
        messageToSend.text[1023] = '\0';

        //Send a message containing the info of the file 
        messageToSend = input.GetMapInfo("output.txt");
        sendSocket.send_to(&messageToSend, sizeof(messageToSend), remoteSocket);

        // Send the contents of the file.
        // I think  the first if its optional, if done correctly,
        // the loop should iterate at least once even if numberOfLoops == 0
        char* aux_pointer = (char*)input.GetMapPointer();
        if (input.GetMapLength() < MAX_PACKAGE_SIZE) {
            sendSocket.send_to(aux_pointer, input.GetMapLength(), remoteSocket);
        }
        else {
            int numberOfLoops = input.GetMapLength() / MAX_PACKAGE_SIZE;
            int aux_length = input.GetMapLength();

            for (int i = 0; i <= numberOfLoops; ++i) {
                if (aux_length < MAX_PACKAGE_SIZE) {
                    sendSocket.send_to(aux_pointer, aux_length, remoteSocket);
                }
                else if (aux_length > 0) {
                    aux_length -= sendSocket.send_to(aux_pointer, MAX_PACKAGE_SIZE, remoteSocket);
                    aux_pointer += MAX_PACKAGE_SIZE;
                }
            }
        }
    }
    catch (std::system_error& e) {
        std::cerr << e.what() << "\n";
    }
    catch (...) {
        eptr = std::current_exception();
    }
}

void NetcpRecieve(std::exception_ptr& eptr, std::string& pathname, std::atomic_bool& pause, std::atomic_bool& abortRecieve) {
    try {

        sockaddr_in recieveSocketAdress(make_ip_address(6660, "127.0.0.1"));
        Socket recieveSocket(recieveSocketAdress);

        // Create directory
        int mkdir_result = mkdir(pathname.c_str(), S_IRWXU);
        if (mkdir_result < 0) {
            throw std::system_error(errno, std::system_category(), "mkdir failed");
        }

        // Save the file information 
        Message messageToRecieve = recieveSocket.recieve_message();

        std::string fullPathname = (std::string(messageToRecieve.text.data()));
        File output(&fullPathname.c_str()[0], messageToRecieve.file_size);

        // Save the file contents 
        int numberOfLoops = messageToRecieve.file_size / MAX_PACKAGE_SIZE;
        char* aux_ptr = (char*)output.GetMapPointer();
        int aux_length = output.GetMapLength();

        for (int i = 0; i <= numberOfLoops; ++i) {
            if (aux_length < MAX_PACKAGE_SIZE) {
                aux_length = recieveSocket.recieve_from(aux_ptr, aux_length);
            }
            else if (aux_length > 0) {
                aux_length -= recieveSocket.recieve_from(aux_ptr, MAX_PACKAGE_SIZE);
                aux_ptr += MAX_PACKAGE_SIZE;
            }
        }
    }
    catch (std::system_error& e) {
        std::cerr << e.what() << "\n";
    }
    catch (...) {
        eptr = std::current_exception();
    }

}

void askForInput() {

    std::atomic_bool exit, pause, abortSend, abortRecieve;
    std::string userInput, pathname, filename;
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
                    NetcpSend(eptr, filename);
                }
            }
            else if (userInput == "recieve") {
                sstream >> pathname;
                pathname = "./out/";
                if (pathname.empty()) {
                    std::cout << "comando incompleto: recieve [PathnameToSaveFile]\n";
                }
                else {
                    std::exception_ptr eptr{};
                    NetcpRecieve(eptr, pathname, pause, abortRecieve);
                }
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