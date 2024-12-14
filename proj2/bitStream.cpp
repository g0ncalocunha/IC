#include <iostream>
#include <fstream>
#include <cstdint>
#include <string>
#include <bitset>

using namespace std;

class bitStream
{
private:
    /* data */
    // file stream
    char bitChar;
    char bitRead;
    char buffer = 0x00;
    int bufSize = 0;
    char rBuffer = 0x00;
    int rBufSize = 0;
    int currentByte = 0;

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
        buffer = buffer | (bit << (7 - (bufSize % 8)));
        bufSize++;
        if (bufSize == 8)
        {
            fs.put(buffer);
            buffer = 0x00;
            bufSize = 0;
        }
    }

    void flushBuffer()
    {
        if (bufSize > 0)
        {
            fs.put(buffer);
            buffer = 0x00;
            bufSize = 0;
        }
    }

    int readBit()
    {
        if (rBufSize == 0)
        {
            fs.clear();  // Limpa qualquer flag de erro
            fs.seekg(currentByte);
            fs.get(rBuffer);
            // cout << "rBuffer: " << bitset<8>(rBuffer) << endl;
            rBufSize = 8;
            // cout << "currentByte: " << currentByte << endl;
        }

        
        int bit = (rBuffer >> (rBufSize - 1)) & 1;
        // cout << "position: " << currentByte * 8 + 1<< " bit: " << bit << " rBufSize: " << rBufSize << endl;
        rBufSize--;
        if (rBufSize == 0)
        {
            // cout << "BUFFER VAZIO" << endl;
            currentByte++;
        }
        return bit;
    }

    void writeBits(uint64_t value, int n)
    {
        if (n <= 0 || n > 64)
        {
            throw invalid_argument("N must be between 0 and 64");
        }
        for (int i = n - 1; i >= 0; i--)
        {
            writeBit((value >> i) & 1);
        }
    }

    uint64_t readBits(int n)
    {
        if (n <= 0 || n > 64)
        {
            throw invalid_argument("N must be between 0 and 64");
        }
        uint64_t value = 0;
        for (int i = 0; i < n; i++)
        {
            value = (value << 1) | readBit();
            if (rBufSize == 0 && fs.peek() != EOF)
            {
                fs.get(rBuffer);
                rBufSize = 8;
            }
        }
        return value;
    }

    void writeString(const string &s)
    {
        for (size_t i = 0; i < s.size(); ++i)
        {
            writeBits(static_cast<uint64_t>(s[i]), 8);
        }
    }
    string readString(int length)
    {
        string s;
        for (size_t i = 0; i < length; ++i)
        {
            char c = static_cast<char>(readBits(8));
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
    if (fs.is_open())
    {
        fs.close();
    }
}

// int main(int argc, char const *argv[])
// {
//     bitStream bs;
//     bs.openFile("test");
//     int n = 7;
//     int quotient = 0;

//     bs.writeBits(0b1111111000000000, 16);
//     bs.flushBuffer();
//     int tempBit = 1;

//     while (bs.readBit() == 1)
//     {
//         quotient++;
//     }

//     cout << "Quotient: " << quotient << endl;

//     uint64_t readValue = bs.readBits(n);
//     cout << bitset<16>(readValue).to_string().substr(16 - n) << endl;

//     return 0;
// }
