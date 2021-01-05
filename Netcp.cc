#include <exception>
#include <thread>
#include <vector>
#include <atomic>
#include <signal.h>
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

        abortSend = true;

    }
    catch (std::system_error& e) {
        std::cerr << e.what() << "\n";
    }
    catch (...) {
        eptr = std::current_exception();
    }

}


void NetcpReceive(std::exception_ptr& eptr, std::string& pathname, std::atomic_bool& abortReceive) {
    try {

        sockaddr_in receiveSocketAdress(make_ip_address(6660, "127.0.0.1"));
        Socket receiveSocket(receiveSocketAdress);

        if (abortReceive) {
            return;
        }

        // Create directory
        mkdir(pathname.c_str(), S_IRWXU);

        if (abortReceive) {
            return;
        }

        while (!abortReceive) {

            if (abortReceive) {
                return;
            }

            //receive the message with the file info
            Message messageToReceive = receiveSocket.receive_message();

            //create and map the file using the received info
            std::string fullPathname = (std::string(messageToReceive.text.data()));
            File output(&fullPathname.c_str()[0], messageToReceive.file_size);

            if (abortReceive) {
                return;
            }

            // .Calculate and set pointers and threshold necessary to receive the contents of the file.
            int numberOfLoops = messageToReceive.file_size / MAX_PACKAGE_SIZE;
            char* aux_ptr = (char*)output.GetMapPointer();
            int aux_length = output.GetMapLength();


            std::cout << "receive nOfLoops:" << numberOfLoops << "\n";

            if (abortReceive) {
                return;
            }
            //Location of error, check conditions of ifs do not change aux_length
            for (int i = 0; i <= numberOfLoops; ++i) {
                if (abortReceive) {
                    return;
                }
                else if (aux_length < MAX_PACKAGE_SIZE) {
                    aux_length = receiveSocket.receive_from(aux_ptr, aux_length);
                }
                else if (aux_length > 0) {
                    aux_length -= receiveSocket.receive_from(aux_ptr, MAX_PACKAGE_SIZE);
                    aux_ptr += MAX_PACKAGE_SIZE;
                }
            }
        }
        return;
    }
    catch (std::system_error& e) {
        if (e.code().value() == EINTR) {
            std::cout << "\nNetcpReceive aborted.";
        }
        else {
            std::cerr << e.what() << "\n";
        }
    }
    catch (...) {
        eptr = std::current_exception();
    }

}


static void SignalHandler(int sig, siginfo_t* siginfo, void* context) {
    std::cout << "\nSignal handler called\n";
    return;
}

void askForInput(std::exception_ptr& eptr) {

    std::atomic_bool exit, pause, abortSend, abortReceive;
    abortSend = true; abortReceive = true;
    std::string userInput, pathname, filename;
    std::thread receiveThread;
    std::vector<std::thread> vecOfThreads;

    struct sigaction act = {};
    act.sa_flags = 0;
    act.sa_sigaction = &SignalHandler;
    sigaction(SIGUSR1, &act, NULL);

    try {

        std::cout << "introduce a command:";
        while (!exit) {
            std::getline(std::cin, userInput);

            std::stringstream sstream(userInput);
            sstream >> userInput;

            if (eptr) {
                std::rethrow_exception(eptr);
            }
            else if (userInput == "quit") {
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

                if (userInput == "receive") {
                    abortReceive = true;
                    pthread_kill(receiveThread.native_handle(), SIGUSR1);

                }
                else {
                    pause = false;
                    abortSend = true;
                }
            }
            else if (userInput == "send") {
                sstream >> filename;
                if (filename.empty()) {
                    std::cout << "\nincomplete instruction: send [FilenameToSend]\n";
                }
                else {

                    if (abortSend) {
                        abortSend = false;
                        vecOfThreads.push_back(std::thread(&NetcpSend, std::ref(eptr), std::ref(filename), std::ref(pause), std::ref(abortSend)));
                    }
                    else {
                        std::cout << "\nWait for the current file to be sent.\n";
                    }
                }
            }
            else if (userInput == "receive") {
                sstream >> pathname;
                pathname = "./out/";
                if (pathname.empty()) {
                    std::cout << "\nincomplete instruction: receive [PathnameToSaveFile]\n";
                }
                else {
                    //check if thread exists already
                    if (abortReceive) {
                        abortReceive = false;
                        receiveThread = std::thread(&NetcpReceive, std::ref(eptr), std::ref(pathname), std::ref(abortReceive));
                    }
                    else {
                        std::cout << "\nYou're already receiving \n";
                    }
                }
            }
            else if (userInput == "help") {
                std::cout << "\nPlaceholder for help message.\n\n";
            }
            else {
                std::cout << "\nUnknown instruction\nintroduce a valid instruction, type \"help\" to display the valid instructions\n\n";
            }
            std::cout << "introduce a command:";
        }

        abortReceive = true;
        pthread_kill(receiveThread.native_handle(), SIGUSR1);

        sleep(1);

        if (receiveThread.joinable()) {
            receiveThread.join();
        }
        for (int i = 0; i < (int)vecOfThreads.size(); i++) {
            if (vecOfThreads[i].joinable()) {
                vecOfThreads[i].join();
            }
        }

    }
    catch (...) {

        abortReceive = true;
        pthread_kill(receiveThread.native_handle(), SIGUSR1);

        abortSend = true;

        sleep(1);

        if (receiveThread.joinable()) {
            receiveThread.join();
        }
        for (int i = 0; i < (int)vecOfThreads.size(); i++) {
            if (vecOfThreads[i].joinable()) {
                vecOfThreads[i].join();
            }
        }
        std::rethrow_exception(eptr);
    }

}


int protected_main() {

    std::exception_ptr eptr{};
    std::vector<std::thread> vecOfThreads;

    std::thread inputThread(&askForInput, std::ref(eptr));
    inputThread.join();
    return 0;
}


int main() {

    try {
        protected_main();
    }
    catch (std::system_error& e) {

        std::cerr << e.what() << "\n";
    }
    return 0;
}