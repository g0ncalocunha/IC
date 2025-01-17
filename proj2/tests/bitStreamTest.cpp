#include "../util/bitStream.cpp"
#include <cassert>
#include <iostream>
#include <cstdint>
#include <vector>

using namespace std;

void testOpenFile()
{
    bitStream bs;
    try
    {
        bs.openFile("test.txt",  ios::binary | ios::in | ios::out | ios::trunc);
        cout << "testOpenFile passed\n";
    }
    catch (const exception &e)
    {
        cerr << "testOpenFile failed: " << e.what() << '\n';
    }
}

void testWriteReadBit()
{
    bitStream bs;
    bs.openFile("test.txt", ios::binary | ios::in | ios::out | ios::trunc);
    bs.writeBit(1);
    bs.writeBit(1);
    bs.writeBit(1);
    bs.writeBit(1);
    bs.writeBit(1);
    bs.writeBit(1);
    bs.writeBit(1);
    bs.writeBit(1);
    int bit = bs.readBit();
    printf("Expected bit: 1, Read bit: %d\n", bit);
    assert(bit == 1);
    cout << "testWriteReadBit passed\n";
}

void testWriteReadBits()
{
    {
        bitStream bs;
        bs.fs.open("test.txt",  ios::binary | ios::in | ios::out | ios::trunc);
        if (!bs.fs.is_open()) {
            throw runtime_error("Failed to open file for writing.");
        }

        uint64_t testValue = 0b11111011;
        int n = 8;

        bs.writeBits(testValue, n);
        bs.fs.close(); 
    }

    {
        bitStream bs;
        bs.fs.open("test.txt", ios::in | ios::binary);
        if (!bs.fs.is_open()) {
            throw runtime_error("Failed to open file for reading.");
        }

        int n = 8;
        uint64_t readValue = bs.readBits(n);

        uint64_t expectedValue = 0b11111011;
        cout << "Expected bits: " << expectedValue << ", Read bits: " << readValue << endl;
        assert(readValue == expectedValue);
        cout << "testWriteReadBits passed\n";

        bs.fs.close();
    }
}

void mixedWriteTest()
{
    bitStream bs;
    bs.fs.open("test.txt",  ios::binary | ios::in | ios::out | ios::trunc);
    if (!bs.fs.is_open()) {
        throw runtime_error("Failed to open file for writing.");
    }

    bs.writeBit(1);
    bs.writeBit(1);
    bs.writeBit(1);
    bs.writeBit(1);
    bs.writeBit(0);
    bs.writeBits(0b010, 3);
    bs.fs.close(); 

    {
        bitStream bs;
        bs.fs.open("test.txt", ios::in | ios::binary);
        if (!bs.fs.is_open()) {
            throw runtime_error("Failed to open file for reading.");
        }

        uint64_t readValue = bs.readBits(8);
        uint64_t expectedValue = 0b11110010;
        cout << "Expected bits: " << expectedValue << ", Read bits: " << readValue << endl;
        assert(readValue == expectedValue);
        cout << "mixedWriteTest passed\n";

        bs.fs.close();
    }

}

void intensiveTestWriteReadBits()
{
    struct TestCase {
        uint64_t testValue;
        int n;
    };

    vector<TestCase> testCases = {
        {0b11111011, 8},
        {0b10101010, 8},
        {0b1111111111111111, 16},
        {0b1010101010101010, 16}
    };

    for (const auto& testCase : testCases) {
        {
            bitStream bs;
            bs.fs.open("test.txt",  ios::binary | ios::in | ios::out | ios::trunc);
            if (!bs.fs.is_open()) {
                throw runtime_error("Failed to open file for writing.");
            }

            bs.writeBits(testCase.testValue, testCase.n);
            bs.fs.close(); 
        }

        {
            bitStream bs;
            bs.fs.open("test.txt", ios::in | ios::binary);
            if (!bs.fs.is_open()) {
                throw runtime_error("Failed to open file for reading.");
            }

            uint64_t readValue = bs.readBits(testCase.n);

            cout << "Expected bits: " << testCase.testValue << ", Read bits: " << readValue << endl;
            assert(readValue == testCase.testValue);
            cout << "Test case passed for n = " << testCase.n << "\n";

            bs.fs.close();
        }
    }
}

void testReadWriteStrings(){
    string testString = "Hello, World!";
    int position = 0;

    {
        bitStream bs;
        bs.fs.open("test.txt",  ios::binary | ios::in | ios::out | ios::trunc);
        if (!bs.fs.is_open()) {
            throw runtime_error("Failed to open file for writing.");
        }
        
        bs.writeString(testString);
        bs.fs.close();
    }

    string readStringResult;
    {
        bitStream bs;
        bs.fs.open("test.txt", ios::in | ios::binary);
        if (!bs.fs.is_open()) {
            throw runtime_error("Failed to open file for reading.");
        }

        readStringResult = bs.readString(testString.length());
        bs.fs.close();
    }

    cout << "Expected: " << testString << ", Read: " << readStringResult << endl;
    assert(readStringResult == testString && "Test case failed for writeString/readString");
    cout << "testWriteReadString passed!\n";
}
void testBulkReadWriteStrings() {
    vector<string> testStrings = {
        "Hello, World!",
        "This is a test string.",
        "Another test string."
    };


    {
        bitStream bs;
        bs.fs.open("test.txt",  ios::binary | ios::in | ios::out | ios::trunc);
        if (!bs.fs.is_open()) {
            throw runtime_error("Failed to open file for writing.");
        }

        for (size_t i = 0; i < testStrings.size(); ++i) {
            bs.writeString(testStrings[i]);
        }
        bs.fs.close();
    }

    vector<string> readStrings;
    {
        bitStream bs;
        bs.fs.open("test.txt", ios::in | ios::binary);
        if (!bs.fs.is_open()) {
            throw runtime_error("Failed to open file for reading.");
        }

        for (size_t i = 0; i < testStrings.size(); ++i) {
            readStrings.push_back(bs.readString(testStrings[i].length()));
        }
        bs.fs.close();
    }

    for (size_t i = 0; i < testStrings.size(); ++i) {
        cout << "Expected: " << testStrings[i] << ", Read: " << readStrings[i] << endl;
        assert(readStrings[i] == testStrings[i] && "Test case failed for writeString/readString");
    }

    cout << "testBulkReadWriteStrings passed!\n";
}

//performance tests
void testPerformance(){
    string testString = "Hello, World!";
    int position = 0;

    {
        bitStream bs;
        bs.fs.open("test.txt",  ios::binary | ios::in | ios::out | ios::trunc);
        if (!bs.fs.is_open()) {
            throw runtime_error("Failed to open file for writing.");
        }
        clock_t start = clock();
        for(int i = 0; i < 1000000; i++){
            bs.writeString(testString);
        }
        clock_t end = clock();
        cout << "Time elapsed for writing 1,000,000 strings: " << end - start << "ms" << endl;
        bs.fs.close();
    }

    {
        bitStream bs;
        bs.fs.open("test.txt", ios::in | ios::binary);
        if (!bs.fs.is_open()) {
            throw runtime_error("Failed to open file for reading.");
        }
        clock_t start = clock();
        for(int i = 0; i < 1000000; i++){
            bs.readString(testString.length());
        }
        clock_t end = clock();
        cout << "Time elapsed for reading 1,000,000 strings: " << end - start << "ms" << endl;
        bs.fs.close();
    }
}

int main()
{
    cout << "Running tests...\n";
    testOpenFile();
    testWriteReadBit();
    testWriteReadBits();
    intensiveTestWriteReadBits();
    testReadWriteStrings();
    testBulkReadWriteStrings();
    mixedWriteTest();
    testPerformance();
    cout << "All tests passed!\n";
    return 0;
}
