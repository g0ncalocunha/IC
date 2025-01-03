#ifndef BITSTREAM_H
#define BITSTREAM_H

#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>

class bitStream {
private:
    char buffer = 0x00;
    int bufSize = 0;
    char rBuffer = 0x00;
    int rBufSize = 0;
    int currentByte = 0;

public:
    std::fstream fs;

    bitStream();
    ~bitStream();

    void openFile(const std::string &filename, std::ios::openmode mode);
    bool isEndOfFile();
    void writeBit(int bit);
    void flushBuffer();
    int readBit();
    void writeBits(uint64_t value, int n);
    uint64_t readBits(int n);
    void writeString(const std::string &s);
    std::string readString(int length);
};

#endif 
