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
        buffer = buffer | (bit << (7 - (bufSize % 8)));
        bufSize++;
        if (bufSize == 8)
        {
            fs.put(buffer);
            buffer = 0x00;
            bufSize = 0x00;
        }
    }

    void flushBuffer()
    {
        if (bufSize > 0)
        {
            fs.put(buffer);
            buffer = 0x00;
            bufSize = 0x00;
        }
    }

    int readBit()
    {
        if (rBufSize == 0)
        {
            fs.clear();  // Limpa qualquer flag de erro
            fs.seekg(0); // Reposiciona o ponteiro de leitura no início do arquivo
            fs.get(rBuffer);
            rBufSize = 8;
        }

        int bit = (rBuffer >> (rBufSize - 1)) & 1;
        rBufSize--;
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

//     bs.writeBit(0);
//     bs.writeBit(0);
//     bs.writeBit(1);
//     bs.writeBit(1);
//     bs.writeBit(0);
//     bs.writeBit(0);
//     bs.writeBit(0);
//     bs.writeBit(1);
//     bs.flushBuffer();
//     int bit = bs.readBit();
//     cout << "Read bit: " << bit << endl;

//     bs.writeBits(0b0011000100110010, 16);
//     bs.flushBuffer();
//     uint64_t readValue = bs.readBits(8);
//     uint64_t expectedValue = 0b00110001;
//     cout << "Expected bits: " << expectedValue << ", Read bits: " << readValue << endl;
//     readValue = bs.readBits(8);
//     expectedValue = 0b00110010;
//     cout << "Expected bits: " << expectedValue << ", Read bits: " << readValue << endl;

//     // string testString = "Hello, World!";
//     // bs.writeString(testString, 0);
//     // string readString = bs.readString(0, testString.size());
//     // cout << "Expected string: " << testString << ", Read string: " << readString << endl;

//     return 0;
// }
