#include "bitStream.cpp"
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
        bs.openFile("test");
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
    bs.openFile("test");
    bs.writeBit(1, 0);
    int bit = bs.readBit(0);
    assert(bit == 1);
    cout << "testWriteReadBit passed\n";
}

void testWriteReadBits()
{
    {
        bitStream bs;
        bs.fs.open("test", ios::out | ios::binary | ios::trunc);
        if (!bs.fs.is_open()) {
            throw runtime_error("Failed to open file for writing.");
        }

        uint64_t testValue = 0b11111011;
        int n = 8;

        bs.writeBits(testValue, n, 0);
        bs.fs.close(); 
    }

    {
        bitStream bs;
        bs.fs.open("test", ios::in | ios::binary);
        if (!bs.fs.is_open()) {
            throw runtime_error("Failed to open file for reading.");
        }

        int n = 8;
        uint64_t readValue = bs.readBits(0, n);

        uint64_t expectedValue = 0b11111011;
        cout << "Expected bits: " << expectedValue << ", Read bits: " << readValue << endl;
        assert(readValue == expectedValue);
        cout << "testWriteReadBits passed\n";

        bs.fs.close();
    }
}

void intensiveTestWriteReadBits()
{
    struct TestCase {
        uint64_t value;
        int bits;
        int pos;
        uint64_t expected;
    };

    vector<TestCase> testCases = {
        {0b10101010, 8, 0, 0b10101010},
        {0b1100, 4, 1, 0b1100},
        {0xFF, 8, 2, 0xFF},
        {0xABCD, 16, 4, 0xABCD},
        {0x12345678, 32, 8, 0x12345678},
        {0xFFFFFFFFFFFFFFFF, 64, 16, 0xFFFFFFFFFFFFFFFF},
        {0b1, 1, 24, 0b1},
        {0xFFFF, 16, 28, 0xFFFF},
        {0xF0F0F0F0F0F0F0F0, 64, 32, 0xF0F0F0F0F0F0F0F0}
    };

    for (const auto& testCase : testCases) {
        {
            bitStream bs;
            bs.fs.open("test", ios::out | ios::binary | ios::trunc);
            if (!bs.fs.is_open()) {
                throw runtime_error("Failed to open file for writing.");
            }
            bs.writeBits(testCase.value, testCase.bits, testCase.pos);
            bs.fs.flush();
            bs.fs.close();
        }

        uint64_t readValue = 0;
        {
            bitStream bs;
            bs.fs.open("test", ios::in | ios::binary);
            if (!bs.fs.is_open()) {
                throw runtime_error("Failed to open file for reading.");
            }
            readValue = bs.readBits(testCase.pos, testCase.bits);
            bs.fs.close();
        }

        cout << "Test case: Writing " << testCase.bits << " bits, value = " << testCase.value
                << " at position " << testCase.pos << endl;
        cout << "Expected: " << testCase.expected << ", Read: " << readValue << endl;

        assert(readValue == testCase.expected && "Test case failed!");
        cout << "Test case passed!\n" << endl;
    }

    cout << "All test cases passed in intensiveTestWriteReadBits!" << endl;
}

void testReadWriteStrings(){
    string testString = "Hello, World!";
    int position = 0;

    {
        bitStream bs;
        bs.fs.open("test", ios::out | ios::binary | ios::trunc);
        if (!bs.fs.is_open()) {
            throw runtime_error("Failed to open file for writing.");
        }
        
        bs.writeString(testString, position);
        bs.fs.close();
    }

    string readStringResult;
    {
        bitStream bs;
        bs.fs.open("test", ios::in | ios::binary);
        if (!bs.fs.is_open()) {
            throw runtime_error("Failed to open file for reading.");
        }

        readStringResult = bs.readString(position, testString.length());
        bs.fs.close();
    }

    cout << "Expected: " << testString << ", Read: " << readStringResult << endl;
    assert(readStringResult == testString && "Test case failed for writeString/readString");
    cout << "testWriteReadString passed!\n";
}
void testBulkReadWriteStrings() {
    struct TestCase {
        string testString;
        int position;
    };

    vector<TestCase> testCases = {
        {"Hello, World!", 0},
        {"A short string.", 50},
        {"A bit longer string with more characters to test the functionality.", 100},
        {string(1024, 'A'), 200},  // 1 KB of 'A'
        {string(8192, 'B'), 1300}, // 8 KB of 'B'
        {string(32768, 'C'), 9500} // 32 KB of 'C'
    };

    for (const auto& testCase : testCases) {
        {
            bitStream bs;
            bs.fs.open("test", ios::out | ios::binary | ios::trunc);
            if (!bs.fs.is_open()) {
                throw runtime_error("Failed to open file for writing.");
            }
            
            bs.writeString(testCase.testString, testCase.position);
            bs.fs.close();
        }

        string readStringResult;
        {
            bitStream bs;
            bs.fs.open("test", ios::in | ios::binary);
            if (!bs.fs.is_open()) {
                throw runtime_error("Failed to open file for reading.");
            }

            readStringResult = bs.readString(testCase.position, testCase.testString.length());
            bs.fs.close();
        }

        // Output and Assertion
        cout << "Test case - Expected: \"" << testCase.testString.substr(0, 20)
                  << (testCase.testString.size() > 20 ? "..." : "")
                  << "\", Read: \"" << readStringResult.substr(0, 20)
                  << (readStringResult.size() > 20 ? "..." : "")
                  << "\"\n";

        assert(readStringResult == testCase.testString && "Test case failed for writeString/readString");
        cout << "Test case passed for string length " << testCase.testString.size() << " at position " << testCase.position << "\n";
    }

    cout << "All bulk test cases passed in testReadWriteStrings!\n";
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
    cout << "All tests passed!\n";
    return 0;
}
