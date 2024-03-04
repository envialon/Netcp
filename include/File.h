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

    void Initialize(const char* pathname, int fileSize, int dirfd);

    void AddToAuxPtr(int offset);
    void SubFromAuxSize(int offset);

public:
    File();
    File(File& input);
    File(const char* pathname);
    File(std::string filename, int fileSize, int dirfd);
    ~File();

    int GetFd();
    void* GetMapPointer();
    // void* GetAuxPtr();
    // int GetAuxLength();
    int GetMapLength();
    Message GetMapInfo(std::string fileName);

    int ReadFile(void* pointer, int size);
    void WriteMappedFile();

};