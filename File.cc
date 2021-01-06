#include "File.h"

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

File::File(const char* pathname, int fileSize, int dirfd) {

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

    map_pointer_ = mmap(NULL, map_length_, PROT_WRITE, MAP_SHARED, fd_, 0);

    if (map_pointer_ == MAP_FAILED) {
        throw std::system_error(errno, std::system_category(), "failure at mapping the file.");
    }
}

File::~File() {
    munmap(map_pointer_, map_length_);

    lockf(fd_, F_ULOCK, 0);
    int close_result = close(fd_);
}


void* File::GetMapPointer() {
    return map_pointer_;
}
int File::GetMapLength() {
    return map_length_;
}
Message File::GetMapInfo(std::string fileName) {
    Message mapInfo{};
    for (int i = 0; i < fileName.length(); i++) {
        mapInfo.text[i] = fileName[i];
    }
    mapInfo.file_size = map_length_;
    return mapInfo;
}

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