#include <iostream>
#include <fstream>
#include <cassert>
#include "golomb.cpp"

void golombTest(const std::string &inputFile, const std::string &outputFile, int m, bool useInterleaving) {
    golomb golomb(m, useInterleaving);
    bitStream bs;

    // Open input file and prepare bitStream for writing
    std::ifstream input(inputFile);
    assert(input.is_open() && "Failed to open input file");
    bs.openFile(outputFile);

    // Encode integers
    int number;
    while (input >> number) {
        golomb.encode(number, bs);
    }
    bs.flushBuffer();
    input.close();

    // Rewind bitStream for reading
    bs.fs.clear();
    bs.fs.seekg(0);

    // Decode integers and verify
    std::ifstream inputVerify(inputFile);
    assert(inputVerify.is_open() && "Failed to open input file for verification");

    // std::cout << "Original -> Decoded:" << std::endl;
    while (inputVerify >> number) {
        int decoded = golomb.decode(bs);
        std::cout << number << " -> " << decoded << std::endl;

        assert(number == decoded && "Mismatch between original and decoded values");
    }

    inputVerify.close();
    bs.fs.close();
}

int main() {
    try {
        std::cout << "Test 1: m = 2, interleaving = true" << std::endl;
        golombTest("input.txt", "output2t.bin", 2, true);

        std::cout << "Test 2: m = 2, interleaving = false" << std::endl;
        golombTest("input.txt", "output2f.bin", 2, false);

        std::cout << "Test 3: m = 5, interleaving = true" << std::endl;
        golombTest("input.txt", "output5t.bin", 5, true);

        std::cout << "Test 4: m = 5, interleaving = false" << std::endl;
        golombTest("input.txt", "output5f.bin", 5, false);

        std::cout << "Test 5: m = 10, interleaving = true" << std::endl;
        golombTest("input.txt", "output10t.bin", 10, true);

        std::cout << "Test 6: m = 10, interleaving = false" << std::endl;
        golombTest("input.txt", "output10f.bin", 10, false);

        std::cout << "Test 7: m = 16, interleaving = true" << std::endl;
        golombTest("input.txt", "output16t.bin", 16, true);

        std::cout << "Test 8: m = 16, interleaving = false" << std::endl;
        golombTest("input.txt", "output16f.bin", 16, false);

        std::cout << "All tests passed!" << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}
