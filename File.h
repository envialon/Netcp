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
    void* map_pointer_;
    int map_length_;

public:
    File(const char* pathname);
    File(const char* pathname, int fileSize, int dirfd);
    ~File();

    int GetFd();
    void* GetMapPointer();
    int GetMapLength();
    Message GetMapInfo(std::string fileName);
    int ReadFile(void* pointer, int size);
    void WriteMappedFile();

};