
#include "Socket.h"
#include "File.h"
#include <exception>

#define MAX_PACKAGE_SIZE 65528

int protected_main(int argc, char* argv[]){
    Socket sendSocket(make_ip_address(0, "127.0.0.1"));
    File input(argv[1]);
    Message messageToSend{};
    messageToSend.text[1023] = '\0';
    sockaddr_in remoteSocket = make_ip_address(6660, "127.0.0.1");
    

    //enviamos la información previa al archivo mapeado
    messageToSend = input.GetMapInfo("outputFile.txt");
    sendSocket.send_to(&messageToSend, sizeof(messageToSend), remoteSocket);

    //enviamos el archivo mapeado
    char* aux_pointer = input.GetMapPointer();
    int aux_length = input.GetMapLength();
    if( aux_length > MAX_PACKAGE_SIZE ){
        sendSocket.send_to(aux_pointer, aux_length, remoteSocket);
    }
    else {    
     while(aux_pointer <= ( input.GetMapPointer() + input.GetMapLength() ) ) {            
            aux_length = sendSocket.send_to(aux_pointer, MAX_PACKAGE_SIZE, remoteSocket);
            aux_pointer += aux_length;
        }
    }
    return 0;
}

int main(int argc, char*argv[]) {

    try{
        return protected_main(argc, argv);
    } 
    catch(std::system_error& e){
        std::cerr << "NetcpRecieve: " << e.what() << '\n';
        return 2;
    }
    catch(...){
        std::cout << "Unknown error \n";
        return 99;
    }
}