#include <array>
#ifndef MESSAGE_H

#define MESSAGE_H
struct Message {
    int file_size;
    std::array<char, 256> text;
};

#endif 