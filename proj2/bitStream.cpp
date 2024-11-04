#include <iostream>
#include <fstream>
#include <string>

class bitStream
{
private:
    /* data */
    // file stream
    std::fstream fs;

public:
    bitStream(/* args */);
    ~bitStream();

    void openFile(const std::string &filename)
    {
        fs.open(filename, fs.binary | fs.in | fs.out);
        if (!fs.is_open())
        {
            throw std::runtime_error("Failed to open file: " + filename);
        }
    }

    void writeBit(int bit, int pos)
    {
        fs.seekp(pos);
        fs.write((char *)&bit, 1);
    }

    int readBit(int pos)
    {
        fs.seekg(pos);
        int bit;
        fs.read((char *)&bit, 1);
        return bit;
    }

    void writeBits(int bits, int n, int pos = 0)
    {
        for (int i = 0; i < n; i++)
        {
            writeBit((bits >> i) & 1, pos + i);
        }
    }

    int readBits(int pos, int n)
    {
        int bits = 0;
        for (int i = 0; i < n; i++)
        {
            bits |= readBit(pos + i) << i;
        }
        return bits;
    }

    void writeString(std::string s)
    {
        for (char c : s)
        {
            writeBits(c, 8);
        }
    }

    std::string readString(int pos = 0)
    {
        std::string s;
        while (true)
        {
            char c = readBits(8, pos);
            if (c == 0)
            {
                break;
            }
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

int main(int argc, char const *argv[])
{
    bitStream bs;
    bs.openFile("test");
    bs.writeBit(1, 0);
    int bit = bs.readBit(0);
    std::cout << "Expected bit: 1, Read bit: " << bit << std::endl;
    return 0;
}

