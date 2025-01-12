#include <iostream>
#include <fstream>
#include <cassert>
#include <chrono>
#include "../golomb.cpp"

void golombTest(const std::string &inputFile, const std::string &outputFile, int m, bool useInterleaving) {
    golomb golomb(m, useInterleaving);
    bitStream bs;

    // Open input file and prepare bitStream for writing
    std::ifstream input(inputFile);
    assert(input.is_open() && "Failed to open input file");
    bs.openFile(outputFile, std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);

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

//performance tests


void performanceTest(const std::string &inputFile, const std::string &outputFile, int m, bool useInterleaving) {
    golomb golomb(m, useInterleaving);
    bitStream bs;

    // Open input file and prepare bitStream for writing
    std::ifstream input(inputFile);
    assert(input.is_open() && "Failed to open input file");
    bs.openFile(outputFile, std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);

    // Measure encoding time
    auto start = std::chrono::high_resolution_clock::now();
    int number;
    while (input >> number) {
        golomb.encode(number, bs);
    }
    bs.flushBuffer();
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> encodingTime = end - start;
    input.close();

    // Rewind bitStream for reading
    bs.fs.clear();
    bs.fs.seekg(0);

    // Measure decoding time
    std::ifstream inputVerify(inputFile);
    assert(inputVerify.is_open() && "Failed to open input file for verification");
    start = std::chrono::high_resolution_clock::now();
    while (inputVerify >> number) {
        int decoded = golomb.decode(bs);
        assert(number == decoded && "Mismatch between original and decoded values");
    }
    end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> decodingTime = end - start;

    inputVerify.close();
    bs.fs.close();

    std::cout << "Performance Test: m = " << m << ", interleaving = " << (useInterleaving ? "true" : "false") << std::endl;
    std::cout << "Encoding time: " << encodingTime.count() << " seconds" << std::endl;
    std::cout << "Decoding time: " << decodingTime.count() << " seconds" << std::endl;
    //size of the output file
    std::ifstream file(outputFile, std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();
    file.close();
    std::cout << "Output file size: " << size << " bytes" << std::endl;
}

int main() {
    try {
        // std::cout << "Test 1: m = 2, interleaving = true" << std::endl;
        // golombTest("input.txt", "output2t.bin", 2, true);

        // std::cout << "Test 2: m = 2, interleaving = false" << std::endl;
        // golombTest("input.txt", "output2f.bin", 2, false);

        // std::cout << "Test 3: m = 5, interleaving = true" << std::endl;
        // golombTest("input.txt", "output5t.bin", 5, true);

        // std::cout << "Test 4: m = 5, interleaving = false" << std::endl;
        // golombTest("input.txt", "output5f.bin", 5, false);

        // std::cout << "Test 5: m = 10, interleaving = true" << std::endl;
        // golombTest("input.txt", "output10t.bin", 10, true);

        // std::cout << "Test 6: m = 10, interleaving = false" << std::endl;
        // golombTest("input.txt", "output10f.bin", 10, false);

        // std::cout << "Test 7: m = 16, interleaving = true" << std::endl;
        // golombTest("input.txt", "output16t.bin", 16, true);

        // std::cout << "Test 8: m = 16, interleaving = false" << std::endl;
        // golombTest("input.txt", "output16f.bin", 16, false);

        //performance tests
        std::cout << "Performance Test 24: m = 4096, interleaving = false" << std::endl;
        performanceTest("input.txt", "output4096f.bin", 4096, false);
        std::cout << "Performance Test 23: m = 4096, interleaving = true" << std::endl;
        performanceTest("input.txt", "output4096t.bin", 4096, true);
        std::cout << "Performance Test 22: m = 2048, interleaving = false" << std::endl;
        performanceTest("input.txt", "output2048f.bin", 2048, false);
        std::cout << "Performance Test 21: m = 2048, interleaving = true" << std::endl;
        performanceTest("input.txt", "output2048t.bin", 2048, true);
        std::cout << "Performance Test 20: m = 1024, interleaving = false" << std::endl;
        performanceTest("input.txt", "output1024f.bin", 1024, false);
        std::cout << "Performance Test 19: m = 1024, interleaving = true" << std::endl;
        performanceTest("input.txt", "output1024t.bin", 1024, true);
        std::cout << "Performance Test 18: m = 512, interleaving = false" << std::endl;
        performanceTest("input.txt", "output512f.bin", 512, false);
        std::cout << "Performance Test 17: m = 512, interleaving = true" << std::endl;
        performanceTest("input.txt", "output512t.bin", 512, true);
        std::cout << "Performance Test 16: m = 256, interleaving = false" << std::endl;
        performanceTest("input.txt", "output256f.bin", 256, false);
        std::cout << "Performance Test 15: m = 256, interleaving = true" << std::endl;
        performanceTest("input.txt", "output256t.bin", 256, true);
        std::cout << "Performance Test 14: m = 128, interleaving = false" << std::endl;
        performanceTest("input.txt", "output128f.bin", 128, false);
        std::cout << "Performance Test 13: m = 128, interleaving = true" << std::endl;
        performanceTest("input.txt", "output128t.bin", 128, true);
        std::cout << "Performance Test 12: m = 64, interleaving = false" << std::endl;
        performanceTest("input.txt", "output64f.bin", 64, false);
        std::cout << "Performance Test 11: m = 64, interleaving = true" << std::endl;
        performanceTest("input.txt", "output64t.bin", 64, true);
        std::cout << "Performance Test 10: m = 32, interleaving = false" << std::endl;
        performanceTest("input.txt", "output32f.bin", 32, false);
        std::cout << "Performance Test 9: m = 32, interleaving = true" << std::endl;
        performanceTest("input.txt", "output32t.bin", 32, true);
        std::cout << "Performance Test 8: m = 16, interleaving = false" << std::endl;
        performanceTest("input.txt", "output16f.bin", 16, false);
        std::cout << "Performance Test 7: m = 16, interleaving = true" << std::endl;
        performanceTest("input.txt", "output16t.bin", 16, true);
        std::cout << "Performance Test 6: m = 10, interleaving = false" << std::endl;
        performanceTest("input.txt", "output10f.bin", 10, false);
        std::cout << "Performance Test 5: m = 10, interleaving = true" << std::endl;
        performanceTest("input.txt", "output10t.bin", 10, true);
        std::cout << "Performance Test 4: m = 5, interleaving = false" << std::endl;
        performanceTest("input.txt", "output5f.bin", 5, false);
        std::cout << "Performance Test 3: m = 5, interleaving = true" << std::endl;
        performanceTest("input.txt", "output5t.bin", 5, true);
        std::cout << "Performance Test 2: m = 2, interleaving = false" << std::endl;
        performanceTest("input.txt", "output2f.bin", 2, false);
        std::cout << "Performance Test 1: m = 2, interleaving = true" << std::endl;
        performanceTest("input.txt", "output2t.bin", 2, true);

        std::cout << "All tests passed!" << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}
