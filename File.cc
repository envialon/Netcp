#include "File.h"


File::File() {

}

File::File(File& input) {

}

File::File(const char* pathname) {

    fd_ = open(pathname, O_RDWR);

    if (fd_ < 0) {
        throw std::system_error(errno, std::system_category(), "failed at opening file");
    }
    lockf(fd_, F_LOCK, 0);

    map_length_ = lseek(fd_, 0, SEEK_END);

    map_pointer_ = mmap(NULL, map_length_, PROT_READ, MAP_SHARED, fd_, 0);

    if (map_pointer_ == MAP_FAILED) {
        throw std::system_error(errno, std::system_category(), "failure at mapping the file.");
    }

}

File::File(std::string filename, int fileSize, int dirfd) {

    fd_ = openat(dirfd, filename.c_str(), O_CREAT | O_RDWR | S_IRWXU, 0666);
    if (fd_ < 0) {
        throw std::system_error(errno, std::system_category(), "failed at opening file");
    }

    lockf(fd_, F_LOCK, 0);

    int ftruncate_result = ftruncate(fd_, (off_t)fileSize);

    if (ftruncate_result < 0) {
        throw std::system_error(errno, std::system_category(), "failure with ftruncate.");
    }

    filename_ = filename;
    map_length_ = fileSize;
    dirfd_ = dirfd;
    map_pointer_ = mmap(NULL, map_length_, PROT_WRITE, MAP_SHARED, fd_, 0);

    if (map_pointer_ == MAP_FAILED) {
        throw std::system_error(errno, std::system_category(), "failure at mapping the file.");
    }
}

File::~File() {
    munmap(map_pointer_, map_length_);
    map_pointer_ = NULL;
    lockf(fd_, F_ULOCK, 0);
    int close_result = close(fd_);
}

void File::initialize(const char* pathname, int fileSize, int dirfd) {
    fd_ = openat(dirfd, pathname, O_CREAT | O_RDWR | S_IRWXU, 0666);
    if (fd_ < 0) {
        throw std::system_error(errno, std::system_category(), "failed at opening file");
    }

    lockf(fd_, F_LOCK, 0);

    int ftruncate_result = ftruncate(fd_, (off_t)fileSize);

    if (ftruncate_result < 0) {
        throw std::system_error(errno, std::system_category(), "failure with ftruncate.");
    }

    map_length_ = fileSize;
    dirfd_ = dirfd;
    map_pointer_ = mmap(NULL, map_length_, PROT_WRITE, MAP_SHARED, fd_, 0);

    if (map_pointer_ == MAP_FAILED) {
        throw std::system_error(errno, std::system_category(), "failure at mapping the file.");
    }
}

int File::GetFd() {
    return fd_;
}

void* File::GetMapPointer() {
    return map_pointer_;
}

// void* File::GetAuxPtr() {
//     return (void*)aux_pointer_;
// }

// int File::GetAuxLength() {
//     return aux_length_;
// }

int File::GetMapLength() {
    return map_length_;
}

Message File::GetMapInfo(std::string fileName) {
    Message mapInfo{};
    strcpy(mapInfo.text.data(), fileName.c_str());
    // for (int i = 0; i < fileName.length(); i++) {
    //     mapInfo.text[i] = fileName[i];
    // }
    mapInfo.file_size = map_length_;
    return mapInfo;
}

// void File::AddToAuxPtr(int offset) {
//     aux_pointer_ += offset;
// }
// void File::SubFromAuxSize(int offset) {
//     aux_length_ -= offset;
// }

int File::ReadFile(void* pointer, int size) {
    int read_result = read(fd_, pointer, size);
    if (read_result < 0) {
        throw std::system_error(errno, std::system_category(), "failed at reading file");
    }
    return read_result;
}

void File::WriteMappedFile() {
    int write_result = write(fd_, map_pointer_, map_length_);
    if (write_result < 0) {
        throw std::system_error(errno, std::system_category(), "failed at reading file");
    }
}