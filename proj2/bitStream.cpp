#include "bitStream.h"
#include <stdexcept>
#include <bitset>

bitStream::bitStream() {}

bitStream::~bitStream() {
    if (fs.is_open()) {
        fs.close();
    }
}

void bitStream::openFile(const std::string &filename, std::ios::openmode mode) {
    fs.open(filename, mode);
    if (!fs.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }
}

bool bitStream::isEndOfFile() {
    return fs.eof();
}

void bitStream::writeBit(int bit) {
    buffer = buffer | (bit << (7 - (bufSize % 8)));
    bufSize++;
    if (bufSize == 8) {
        fs.put(buffer);
        buffer = 0x00;
        bufSize = 0;
    }
}

void bitStream::flushBuffer() {
    if (bufSize > 0) {
        fs.put(buffer);
        buffer = 0x00;
        bufSize = 0;
    }
}

int bitStream::readBit() {
    if (rBufSize == 0) {
        fs.clear();
        fs.seekg(currentByte);
        fs.get(rBuffer);
        rBufSize = 8;
    }
    int bit = (rBuffer >> (rBufSize - 1)) & 1;
    rBufSize--;
    if (rBufSize == 0) {
        currentByte++;
    }
    return bit;
}

void bitStream::writeBits(uint64_t value, int n) {
    if (n <= 0 || n > 64) {
        throw std::invalid_argument("N must be between 0 and 64");
    }
    for (int i = n - 1; i >= 0; i--) {
        writeBit((value >> i) & 1);
    }
}

uint64_t bitStream::readBits(int n) {
    if (n <= 0 || n > 64) {
        throw std::invalid_argument("N must be between 0 and 64");
    }
    uint64_t value = 0;
    for (int i = 0; i < n; i++) {
        value = (value << 1) | readBit();
        if (rBufSize == 0 && fs.peek() != EOF) {
            fs.get(rBuffer);
            rBufSize = 8;
        }
    }
    return value;
}

void bitStream::writeString(const std::string &s) {
    for (size_t i = 0; i < s.size(); ++i) {
        writeBits(static_cast<uint64_t>(s[i]), 8);
    }
}

std::string bitStream::readString(int length) {
    std::string s;
    for (size_t i = 0; i < length; ++i) {
        char c = static_cast<char>(readBits(8));
        s += c;
    }
    return s;
}
