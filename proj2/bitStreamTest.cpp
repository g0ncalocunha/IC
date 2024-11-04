#include "bitStream.cpp"
#include <cassert>
#include <iostream>

void testOpenFile()
{
    bitStream bs;
    try
    {
        bs.openFile("test");
        std::cout << "testOpenFile passed\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << "testOpenFile failed: " << e.what() << '\n';
    }
}

void testWriteReadBit()
{
    bitStream bs;
    bs.openFile("test");
    bs.writeBit(1, 0);
    int bit = bs.readBit(0);
    std::cout << "Expected bit: 1, Read bit: " << bit << std::endl;
    assert(bit == 1);
    std::cout << "testWriteReadBit passed\n";
}

void testWriteReadBits()
{
    bitStream bs;
    bs.openFile("test");
    bs.writeBits(0b10101010, 8);
    int bits = bs.readBits(8, 0);
    std::cout << "Expected bits: 0b10101010, Read bits: " << bits << std::endl;
    assert(bits == 0b10101010);
    std::cout << "testWriteReadBits passed\n";
}

int main()
{
    std::cout << "Running tests...\n";
    testOpenFile();
    testWriteReadBit();
    testWriteReadBits();
    std::cout << "All tests passed!\n";
    return 0;
}

