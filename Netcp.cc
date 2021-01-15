#include <exception>
#include <thread>
#include <vector>
#include <stack>
#include <dirent.h>
#include <mutex>
#include <sys/types.h>
#include <atomic>
#include <signal.h>
#include <sstream>
#include <chrono>
#include "File.h"
#include "Socket.h"
#include <unordered_map>

#define MAX_PACKAGE_SIZE 60000


void NetcpSend(std::exception_ptr& eptr, std::string& filename, std::atomic_bool& pause, std::atomic_bool& abortSend, std::mutex& pauseMutex) {
    try {

        const char* envDestIp = std::getenv("NETCP_DEST_IP");
        const char* envDestPort = std::getenv("NETCP_DEST_PORT");

        if (envDestIp == NULL || envDestPort == NULL) {
            const char* envDestIP = "127.0.0.1";
            const char* envDestPort = "6660";
        }
        Socket sendSocket(make_ip_address(0, "127.0.0.1"));
        sockaddr_in remoteSocket = make_ip_address(atoi(envDestPort), envDestIp);

        File input(filename.c_str());

        Message messageToSend{};
        messageToSend.text[1023] = '\0';

        //Send a message containing the info of the file 
        messageToSend = input.GetMapInfo(filename);
        sendSocket.send_to(&messageToSend, sizeof(messageToSend), remoteSocket);

        // Send the contents of the file.
        char* aux_pointer = (char*)input.GetMapPointer();
        int numberOfLoops = input.GetMapLength() / MAX_PACKAGE_SIZE;
        int aux_length = input.GetMapLength();

        // Number for loop will do at least 1 loop, 
        // ( i <= numberOfLoops ) this way, we will
        // also send the last package,
        // with size < MAX_PACKAGE_SIZE
        for (int i = 0; i <= numberOfLoops; ++i) {

            if (pause) {
                pauseMutex.lock();
                pauseMutex.unlock();
            }
            else if (abortSend) {
                std::cout << "\tNetcpSend aborted.\n";
                return;
            }
            else {
                std::this_thread::sleep_for(std::chrono::milliseconds(3));
                if (aux_length < MAX_PACKAGE_SIZE) {
                    sendSocket.send_to(aux_pointer, aux_length, remoteSocket);
                }
                else if (aux_length > 0) {
                    sendSocket.send_to(aux_pointer, MAX_PACKAGE_SIZE, remoteSocket);
                    aux_length -= MAX_PACKAGE_SIZE;
                    aux_pointer += MAX_PACKAGE_SIZE;
                }
            }
        }

        std::cout << "\n\tFile \"" << filename << "\" sent\n";
        abortSend = true;
        return;
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


        const char* envPort = std::getenv("NETCP_PORT");

        if (envPort == NULL) {
            envPort = "6660";
        }

        sockaddr_in receiveSocketAdress(make_ip_address(atoi(envPort), "127.0.0.1"));
        Socket receiveSocket(receiveSocketAdress);

        // Create directory
        mkdir(pathname.c_str(), S_IRWXU);

        DIR* dirPointer = opendir(pathname.c_str());
        int dirFileDescriptor = dirfd(dirPointer);

        while (!abortReceive) {

            //receive the message with the file info
            Message messageToReceive = receiveSocket.receive_message();

            if (abortReceive) {
                std::cout << "\tNetcpReceive aborted.\n";
                return;
            }

            //create and map the file using the received info
            std::string filename = (std::string(messageToReceive.text.data()));
            File output(&filename.c_str()[0], messageToReceive.file_size, dirFileDescriptor);

            //Calculate and set pointer, length and threshold necessary to receive the contents of the file.
            int numberOfLoops = messageToReceive.file_size / MAX_PACKAGE_SIZE;
            char* aux_ptr = (char*)output.GetMapPointer();
            int aux_length = output.GetMapLength();


            // Number for loop will do at least 1 loop, 
            // ( i <= numberOfLoops ) this way, we will
            // also receive the last  package, 
            // with size < MAX_PACKAGE_SIZE
            for (int i = 0; i <= numberOfLoops; ++i) {
                if (abortReceive) {
                    std::cout << "\tNetcpReceive aborted\n";
                    return;
                }
                else if (aux_length < MAX_PACKAGE_SIZE) {
                    aux_length -= receiveSocket.receive_from(aux_ptr, aux_length);
                }
                else if (aux_length > 0) {
                    receiveSocket.receive_from(aux_ptr, MAX_PACKAGE_SIZE);
                    aux_length -= MAX_PACKAGE_SIZE;
                    aux_ptr += MAX_PACKAGE_SIZE;
                }
            }
            std::cout << "\tFile \"" << filename << "\" saved at " << pathname << "\n";
        }
        std::cout << "\tNetcpReceive aborted.\n";
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
    std::cout << "\tSignal handler called\n";
    return;
}


void PopThread(std::stack<std::thread>& stack) {
    // we use this function to erase the top thread
    // of a stack after waiting for it to finish
    if (!stack.empty() && stack.top().joinable()) {
        stack.top().join();
        stack.pop();
    }
    else {
        return;
    }
}

struct sendTask {
    sendTask(std::thread& thread) : sendThread{ std::move(thread) } {}
    sendTask() : sendThread{ std::move(std::thread()) } {}
    std::atomic_bool abortSend;
    std::atomic_bool pauseSend;
    std::mutex pauseMutex;
    std::thread sendThread;
};

void askForInput(std::exception_ptr& eptr, std::atomic_bool& exit) {

    std::atomic_bool abortReceive;

    abortReceive = true;
    std::string userInput, pathname, filename;

    int sendTaskCount = 0;
    std::unordered_map<int, sendTask> mapOfTasks;
    std::stack<std::thread>  receiveStack;

    struct sigaction act = {};
    act.sa_flags = 0;
    act.sa_sigaction = &SignalHandler;
    sigaction(SIGUSR1, &act, NULL);

    try {
        while (!exit) {

            // This cout is not synchronized wiht the messages of the other threads.
            std::cout << "Introduce a command:";
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
                sstream >> userInput;
                int index = atoi(userInput.c_str());
                if (mapOfTasks.find(index) != mapOfTasks.end()) {
                    mapOfTasks[index].pauseSend = false;
                    mapOfTasks[index].pauseMutex.unlock();
                }
            }

            else if (userInput == "pause") {
                sstream >> userInput;
                int index = atoi(userInput.c_str());
                if (mapOfTasks.find(index) != mapOfTasks.end()) {
                    mapOfTasks[index].pauseSend = true;
                    mapOfTasks[index].pauseMutex.try_lock();
                }
            }

            else if (userInput == "abort") {
                sstream >> userInput;

                if (userInput == "receive") {
                    if (!receiveStack.empty()) {
                        abortReceive = true;
                        pthread_kill(receiveStack.top().native_handle(), SIGUSR1);
                        PopThread(receiveStack);
                    }
                    else {
                        std::cout << "\n\tYou can't abort something that doesn't exist...\n";
                    }
                }
                else {
                    int index = atoi(userInput.c_str());
                    if (mapOfTasks.find(index) != mapOfTasks.end()) {
                        mapOfTasks[index].abortSend = true;
                        mapOfTasks[index].pauseSend = false;
                        mapOfTasks[index].pauseMutex.unlock();
                    }
                    else {
                        std::cout << "\n\tYou can't abort something that doesn't exist...\n";
                    }
                }
            }

            else if (userInput == "send") {
                sstream >> filename;
                if (filename.empty()) {
                    std::cout << "\n\tIncomplete instruction: send [FilenameToSend]\n";
                }

                //if we can read the file, proceed to send it.
                else if (access(filename.c_str(), F_OK) == 0) {

                    std::thread aux;
                    mapOfTasks.emplace(sendTaskCount, aux);
                    mapOfTasks[sendTaskCount].sendThread = std::thread(&NetcpSend, std::ref(eptr),
                        std::ref(filename), std::ref(mapOfTasks[sendTaskCount].pauseSend),
                        std::ref(mapOfTasks[sendTaskCount].abortSend), std::ref(mapOfTasks[sendTaskCount].pauseMutex));

                    std::cout << "\nNew send created with id: " << sendTaskCount << "\n";
                    sendTaskCount++;
                }
                else {
                    std::cout << "\t" << filename << " doesn't exist.\n";
                }
            }

            else if (userInput == "receive") {
                sstream >> pathname;
                if (pathname.empty()) {
                    std::cout << "\n\tIncomplete instruction: receive [PathnameToSaveFile]\n";
                }
                else {
                    //check if thread exists already
                    if (abortReceive) {
                        PopThread(receiveStack);
                        abortReceive = false;
                        receiveStack.push(std::thread(&NetcpReceive, std::ref(eptr), std::ref(pathname), std::ref(abortReceive)));
                    }
                    else {
                        std::cout << "\nYou're already receiving\n";
                    }
                }
            }

            else if (userInput == "help") {
                std::cout << "\nPlaceholder for help message.\n\n";
            }

            else if (!exit) {
                std::cout << "\n\tUnknown instruction\n\tIntroduce a valid instruction, type \"help\" to display the valid instructions\n\n";
            }
        }


        abortReceive = true;

        for (int i = 0; i < (int)receiveStack.size(); i++) {
            pthread_kill(receiveStack.top().native_handle(), SIGUSR1);
            sleep(1);
            PopThread(receiveStack);
        }

    }

    catch (...) {
        abortReceive = true;

        for (int i = 0; i < (int)receiveStack.size(); i++) {
            pthread_kill(receiveStack.top().native_handle(), SIGUSR1);
            sleep(1);
            PopThread(receiveStack);
        }
        eptr = std::current_exception();
    }
}


void auxThread(pthread_t& threadToKill, sigset_t& sigwaitset, std::atomic_bool& inputExit) {
    int signum;
    sigwait(&sigwaitset, &signum);
    if (signum == SIGINT || signum == SIGTERM || signum == SIGHUP) {
        inputExit = true;
        pthread_kill(threadToKill, SIGUSR1);
    }
}


int protected_main() {

    std::exception_ptr eptr{};
    std::atomic_bool exit;
    sigset_t sigwaitset;

    sigemptyset(&sigwaitset);
    sigaddset(&sigwaitset, SIGINT);
    sigaddset(&sigwaitset, SIGTERM);
    sigaddset(&sigwaitset, SIGHUP);

    pthread_sigmask(SIG_SETMASK, &sigwaitset, NULL);

    std::thread inputThread(&askForInput, std::ref(eptr), std::ref(exit));
    pthread_t inputThreadNativeHandle = inputThread.native_handle();

    std::thread sigThread(&auxThread, std::ref(inputThreadNativeHandle), std::ref(sigwaitset), std::ref(exit));
    sigThread.detach();

    inputThread.join();

    if (eptr) {
        std::rethrow_exception(eptr);
    }

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