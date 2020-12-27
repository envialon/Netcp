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
        File input(filename.c_str());
        Message messageToSend{};
        messageToSend.text[1023] = '\0';
        const sockaddr_in remoteSocket = make_ip_address(6660, "127.0.0.1");

        // enviamos la información previa al archivo mapeado
        messageToSend = input.GetMapInfo("output.txt");
        sendSocket.send_to(&messageToSend, sizeof(messageToSend), remoteSocket);

        // enviamos el archivo mapeado
        char* aux_pointer = (char*)input.GetMapPointer();

        char* whileThreshold = (char*)(input.GetMapPointer()) + (input.GetMapLength());
        std::cout << "send1\n";
        while (aux_pointer <= whileThreshold) {
            sleep(5);
            std::cout << "send2\n";
            //problem is here, have to calculate the exact number for the sendto size.
            sendSocket.send_to(aux_pointer, (size_t)MAX_PACKAGE_SIZE, remoteSocket);
            aux_pointer += MAX_PACKAGE_SIZE;
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

        // Get first message, with the necessary information needed to create the file.
        Message messageToRecieve = recieveSocket.recieve_message();
        std::cout << "1\n";
        // Create and map the file in memory
        std::string fullPathname = (std::string(messageToRecieve.text.data()));
        File output(&fullPathname.c_str()[0], messageToRecieve.file_size);
        std::cout << "2\n";
        // read the content of the file in the mapped memory of the file 
        char* aux_ptr = (char*)output.GetMapPointer();
        const char* outputThreshold = (char*)(output.GetMapPointer()) + (output.GetMapLength());
        std::cout << "3\n";
        //wait for all the packages and store them in the mapped file.
        while (aux_ptr < outputThreshold) {
            std::cout << "4\n";
            recieveSocket.recieve_from((void*)aux_ptr, MAX_PACKAGE_SIZE);
            aux_ptr += MAX_PACKAGE_SIZE;
        }

        output.WriteMappedFile();
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