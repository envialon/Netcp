#include <iostream>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "Message.h"

class File {

private:
    int fd_;
    int dirfd_;
    void* map_pointer_;
    // char* aux_pointer_;
    // int aux_length_;
    int map_length_;
    std::string filename_;

public:
    File();
    File(File& input);
    File(const char* pathname);
    File(std::string filename, int fileSize, int dirfd);
    ~File();

    void initialize(const char* pathname, int fileSize, int dirfd);

    int GetFd();
    void* GetMapPointer();
    // void* GetAuxPtr();
    // int GetAuxLength();
    int GetMapLength();
    Message GetMapInfo(std::string fileName);

    void AddToAuxPtr(int offset);
    void SubFromAuxSize(int offset);

    int ReadFile(void* pointer, int size);
    void WriteMappedFile();

};