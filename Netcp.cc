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


void NetcpRecieve(std::exception_ptr& eptr, std::string& pathname, std::atomic_bool& abortRecieve) {
    try {

        sockaddr_in recieveSocketAdress(make_ip_address(6660, "127.0.0.1"));
        Socket recieveSocket(recieveSocketAdress);

        if (abortRecieve) {
            return;
        }

        // Create directory
        mkdir(pathname.c_str(), S_IRWXU);

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
            //Location of error, check conditions of ifs do not change aux_length
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
    catch (std::system_error& e) {
        std::cerr << e.what() << "\n";
    }
    catch (...) {
        eptr = std::current_exception();
    }

}


static void SignalHandler(int sig, siginfo_t* siginfo, void* context) {
    std::cout << "signal handler called\n";
    return;
}

void askForInput(std::exception_ptr& eptr) {

    std::atomic_bool exit, pause, abortSend, abortRecieve;
    abortSend = true; abortRecieve = true;
    std::string userInput, pathname, filename;
    std::thread recieveThread;
    std::vector<std::thread> vecOfThreads;

    struct sigaction act = {};
    act.sa_flags = 0;
    act.sa_sigaction = &SignalHandler;
    sigaction(SIGUSR1, &act, NULL);

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
                    pthread_kill(recieveThread.native_handle(), SIGUSR1);

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
            else if (userInput == "recieve") {
                sstream >> pathname;
                pathname = "./out/";
                if (pathname.empty()) {
                    std::cout << "\nincomplete instruction: recieve [PathnameToSaveFile]\n";
                }
                else {

                    //check if thread exists already
                    if (abortRecieve) {
                        abortRecieve = false;
                        recieveThread = std::thread(&NetcpRecieve, std::ref(eptr), std::ref(pathname), std::ref(abortRecieve));
                    }
                }
            }
            else if (userInput == "help") {
                std::cout << "\nplaceholder for help message.\n\n";
            }
            else {
                std::cout << "\nUnknown instruction\nintroduce a valid instruction, type \"help\" to display the valid instructions\n\n";
            }
        }

        abortRecieve = true;
        pthread_kill(recieveThread.native_handle(), SIGUSR1);

        sleep(1);

        if (recieveThread.joinable()) {
            recieveThread.join();
        }
        for (int i = 0; i < (int)vecOfThreads.size(); i++) {
            if (vecOfThreads[i].joinable()) {
                vecOfThreads[i].join();
            }
        }

    }
    catch (...) {

        abortRecieve = true;
        pthread_kill(recieveThread.native_handle(), SIGUSR1);

        abortSend = true;

        sleep(1);

        if (recieveThread.joinable()) {
            recieveThread.join();
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