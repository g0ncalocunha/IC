#include <iostream>
#include <fstream>
#include <cstdint>
#include <string>

using namespace std;

class bitStream
{
private:
    /* data */
    // file stream
    char bitChar;
    char bitRead;
    char buffer = 0x00;
    char bufSize = 0x00;
    char rBuffer = 0x00;
    char rBufSize = 0x00;

public:
    fstream fs;
    bitStream(/* args */);
    ~bitStream();

    void openFile(const string &filename)
    {
        fs.open(filename, fs.binary | fs.in | fs.out | fs.trunc);
        if (!fs.is_open())
        {
            throw runtime_error("Failed to open file: " + filename);
        }
    }

    void writeBit(int bit)
    {
        buffer = buffer | (bit << (bufSize % 8));
        bufSize++;
        if (bufSize == 8) {
            fs.put(buffer);
            buffer = 0x00;
            bufSize = 0x00;
        }
    }

    void flushBuffer() {
        fs.put(buffer);
    }

    int readBit() {   
        if (rBufSize == 0) {
            fs.get(rBuffer);
            rBufSize = 8;
        }

        int bit = (rBuffer >> (rBufSize - 1)) & 1;
        rBufSize--;
        return bit;
    }

    void writeBits(uint64_t value, int n, int pos = 0)
    {
        if (n <= 0 || n > 64) {
            throw invalid_argument("N must be between 0 and 64");
        }        
        
        int byteCount = (n + 7) / 8;
        fs.seekp(pos);
        if (!fs) {
            throw runtime_error("Failed to seek to position " + to_string(pos));
        }
        fs.clear();
        for (int i = 0; i < byteCount; i++){
            unsigned char byte = (value >> (i * 8)) & 0xFF; // Extracts a byte
            fs.write(reinterpret_cast<char*>(&byte), 1);
        }
        fs.flush();
    }

    uint64_t readBits(int pos, int n)
    {
        if (n <= 0 || n > 64) {
            throw invalid_argument("N must be between 0 and 64");
        }
        uint64_t bits = 0;
        int byteCount = (n + 7) / 8; // byte number
        fs.clear();
        fs.seekg(pos);
        if (!fs) {
            throw runtime_error("Failed to seek to position " + to_string(pos));
        }

        for (int i = 0; i < byteCount; i++)
        {
            unsigned char byte;
            fs.read(reinterpret_cast<char*>(&byte), 1);
            if (!fs) {
                cout << "Read failed at byte position: " << (pos + i) << endl;
                break;
            }
            bits |= static_cast<uint64_t>(byte) << (i * 8);
        }
        if (n < 64) {
            bits &= (1ULL << n) - 1;
        }
        return bits;
    }

    void writeString(string s, int pos)
    {
        for (size_t i = 0; i < s.size(); ++i) {
            writeBits(static_cast<uint64_t>(s[i]), 8, pos + i * 8);
        }
    }

    string readString(int pos, int length) 
    {
        string s;
        for (size_t i = 0; i < length; ++i) {
            char c = static_cast<char>(readBits(pos + i * 8, 8));
            s += c;
        }
        return s;
    }

};

bitStream::bitStream(/* args */)
{
}

bitStream::~bitStream()
{
}


// int main(int argc, char const *argv[])
// {
//     bitStream bs;
//     bs.openFile("test");
//     bs.writeBit(1, 0);
//     int bit = bs.readBit(0);
//     cout << "Expected bit: 1, Read bit: " << bit << endl;
//     return 0;
// }

