#include <exception>
#include <thread>
#include <vector>
#include <atomic>
#include <sstream>
#include "File.h"
#include "Socket.h"

#define MAX_PACKAGE_SIZE 60000

void NetcpSend(std::exception_ptr& eptr, std::string& filename, std::atomic_bool& pause, std::atomic_bool& abortSend) {
    try {

        Socket sendSocket(make_ip_address(0, "127.0.0.1"));
        sockaddr_in remoteSocket = make_ip_address(6660, "127.0.0.1");

        File input(filename.c_str());

        Message messageToSend{};
        messageToSend.text[1023] = '\0';

        if (abortSend) {
            return;
        }
        //Send a message containing the info of the file 
        messageToSend = input.GetMapInfo("output.txt");
        sendSocket.send_to(&messageToSend, sizeof(messageToSend), remoteSocket);

        if (abortSend) {
            return;
        }
        // Send the contents of the file.
        char* aux_pointer = (char*)input.GetMapPointer();
        int numberOfLoops = input.GetMapLength() / MAX_PACKAGE_SIZE;
        int aux_length = input.GetMapLength();

        if (abortSend) {
            return;
        }

        for (int i = 0; i <= numberOfLoops; ++i) {

            if (abortSend) {
                return;
            }
            else if (pause) {
                i--;
            }
            else {
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
    catch (...) {
        eptr = std::current_exception();
    }

}

void NetcpRecieve(std::exception_ptr& eptr, std::string& pathname, std::atomic_bool& abortRecieve) {
    try {

        sockaddr_in recieveSocketAdress(make_ip_address(6660, "127.0.0.1"));
        Socket recieveSocket(recieveSocketAdress);

        if (abortRecieve) {
            return;
        }

        // Create directory
        int mkdir_result = mkdir(pathname.c_str(), S_IRWXU);
        if (mkdir_result < 0) {
            throw std::system_error(errno, std::system_category(), "mkdir failed");
        }

        if (abortRecieve) {
            return;
        }

        while (!abortRecieve) {

            if (abortRecieve) {
                return;
            }

            //recieve the message with the file info
            Message messageToRecieve = recieveSocket.recieve_message();

            //create and map the file using the recieved info
            std::string fullPathname = (std::string(messageToRecieve.text.data()));
            File output(&fullPathname.c_str()[0], messageToRecieve.file_size);

            if (abortRecieve) {
                return;
            }

            // .Calculate and set pointers and threshold necessary to recieve the contents of the file.
            int numberOfLoops = messageToRecieve.file_size / MAX_PACKAGE_SIZE;
            char* aux_ptr = (char*)output.GetMapPointer();
            int aux_length = output.GetMapLength();


            std::cout << "recieve nOfLoops:" << numberOfLoops << "\n";

            if (abortRecieve) {
                return;
            }

            for (int i = 0; i <= numberOfLoops; ++i) {
                if (abortRecieve) {
                    return;
                }
                else if (aux_length < MAX_PACKAGE_SIZE) {
                    aux_length = recieveSocket.recieve_from(aux_ptr, aux_length);
                }
                else if (aux_length > 0) {
                    aux_length -= recieveSocket.recieve_from(aux_ptr, MAX_PACKAGE_SIZE);
                    aux_ptr += MAX_PACKAGE_SIZE;
                }
            }
        }
        return;
    }
    catch (...) {
        eptr = std::current_exception();
    }

}

void askForInput() {

    std::atomic_bool exit, pause, abortSend, abortRecieve;
    std::string userInput, pathname, filename;
    std::exception_ptr eptr{};
    std::thread sendThread, recieveThread;

    try {
        while (!exit) {
            std::cout << "introduce a command:";
            std::getline(std::cin, userInput);

            std::stringstream sstream(userInput);
            sstream >> userInput;

            if (eptr) {
                std::rethrow_exception(eptr);
            }
            else if (userInput == "quit") {
                exit = true;
                abortRecieve = true;
                abortSend = true;
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
                    recieveThread.join();
                }
                else {
                    abortSend = true;
                    sendThread.join();
                }
            }
            else if (userInput == "send") {
                sstream >> filename;
                if (filename.empty()) {
                    std::cout << "\nincomplete instruction: send [FilenameToSend]\n";
                }
                else {
                    sendThread = std::thread(&NetcpSend, std::ref(eptr), std::ref(filename), std::ref(pause), std::ref(abortSend));
                }
            }
            else if (userInput == "recieve") {
                sstream >> pathname;
                pathname = "./out/";
                if (pathname.empty()) {
                    std::cout << "\nincomplete instruction: recieve [PathnameToSaveFile]\n";
                }
                else {
                    recieveThread = std::thread(&NetcpRecieve, std::ref(eptr), std::ref(pathname), std::ref(abortRecieve));
                }
            }
            else if (userInput == "help") {
                std::cout << "\nplaceholder for help message.\n\n";
            }
            else {
                std::cout << "\nUnknown instruction\nintroduce a valid instruction, type \"help\" to display the valid instructions\n\n";
            }
        }

        if (sendThread.joinable()) {
            sendThread.join();
        }
        if (recieveThread.joinable()) {
            recieveThread.join();
        }


    }
    catch (std::system_error& e) {

        abortRecieve = true;
        abortSend = true;

        if (sendThread.joinable()) {
            sendThread.join();
        }
        if (recieveThread.joinable()) {
            recieveThread.join();
        }

        std::cerr << e.what() << "\n";
    }

}

int main() {
    askForInput();
    return 0;
}