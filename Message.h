#include <array>
#ifndef MESSAGE_H

#define MESSAGE_H
struct Message {
    std::array<char, 256> text;
    int file_size;
};

#endif 