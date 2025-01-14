#include <string>
#include <vector>
#include <cstdint>
#include <cmath>
#include <stdexcept>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <memory>
#include <SFML/Audio.hpp>
#include "../util/bitStream.h"
#include "../util/golomb.h"

class AudioCodec {
private:
    static constexpr int BLOCK_SIZE = 1024;
    static constexpr int MAX_QUANT_LEVEL = 255;
    static constexpr int MIN_QUANT_LEVEL = 1;
    static constexpr double PI = 3.14159265358979323846;
    
    int quantizationLevel;
    bool adaptive;
    bool lossy;
    double targetBitrate;
    sf::SoundBuffer buffer;
    std::unique_ptr<golomb> coder;

    uint32_t calculateChecksum(const sf::Int16* samples, size_t size) {
        uint32_t checksum = 0;
        for (size_t i = 0; i < size; i++) {
            checksum = (checksum << 1) ^ static_cast<uint32_t>(samples[i]);
        }
        return checksum;
    }

    // Verification of lossless reconstruction
    bool verifyBlock(const sf::Int16* original, const sf::Int16* decoded, size_t size) {
        for (size_t i = 0; i < size; i++) {
            if (original[i] != decoded[i]) {
                return false;
            }
        }
        return true;
    }

    void dct(const std::vector<double>& input, std::vector<double>& output) {
        const size_t N = input.size();
        output.resize(N);
        
        for (size_t k = 0; k < N; k++) {
            double sum = 0.0;
            const double scale = (k == 0) ? std::sqrt(1.0/N) : std::sqrt(2.0/N);
            
            for (size_t n = 0; n < N; n++) {
                sum += input[n] * std::cos(PI * k * (2.0 * n + 1) / (2.0 * N));
            }
            
            output[k] = scale * sum;
        }
    }

    void idct(const std::vector<double>& input, std::vector<double>& output) {
        const size_t N = input.size();
        output.resize(N);
        
        for (size_t n = 0; n < N; n++) {
            double sum = input[0] / std::sqrt(N);
            
            for (size_t k = 1; k < N; k++) {
                sum += std::sqrt(2.0/N) * input[k] * 
                       std::cos(PI * k * (2.0 * n + 1) / (2.0 * N));
            }
            
            output[n] = sum;
        }
    }

     void encodeLossless(const sf::Int16* samples, size_t blockSize, bitStream& bs) {
        if (blockSize == 0) return;

        // Block checksum for verification
        uint32_t checksum = calculateChecksum(samples, blockSize);
        bs.writeBits(checksum, 32);

        bs.writeBits(static_cast<uint16_t>(samples[0] + 32768), 16);

        int k;
        if (adaptive && blockSize > 1) {
            // Calculate mean absolute difference for better k parameter estimation
            double sum = 0;
            int maxDelta = 0;
            for (size_t i = 1; i < blockSize; i++) {
                int delta = std::abs(samples[i] - samples[i-1]);
                sum += delta;
                maxDelta = std::max(maxDelta, delta);
            }
            
            double mean = sum / (blockSize - 1);
            if (mean > 0) {
                k = static_cast<int>(std::log2(mean * 0.69314718)); //optimal Rice coding
            } else {
                k = 0;
            }
            
            // Adjust k based on maximum delta 
            int maxBits = static_cast<int>(std::log2(maxDelta + 1));
            k = std::min(k, maxBits);
            k = std::clamp(k, 0, 12);  
            bs.writeBits(k, 4);
        } else {
            k = 8;  // Default -> non-adaptive mode
        }

        int16_t prevSample = samples[0];
        int16_t prevDelta = 0;

        for (size_t i = 1; i < blockSize; i++) {
            int32_t predicted = prevSample + prevDelta;
            int32_t actual = samples[i];
            int32_t delta = actual - predicted;
            
            prevDelta = actual - prevSample;
            prevSample = actual;

            uint32_t uValue;
            if (delta < 0) {
                uValue = (-2 * delta - 1);
            } else {
                uValue = (2 * delta);
            }
            
            uint32_t q = uValue >> k;
            uint32_t r = uValue & ((1 << k) - 1);
            
            while (q > 31) {  
                bs.writeBits(0x7FFFFFFF, 31);  // Write 31 ones
                q -= 31;
            }
            if (q > 0) {
                bs.writeBits((1 << q) - 1, q);
            }
            bs.writeBit(0);
            
            if (k > 0) {  // Only write remainder if k > 0
                bs.writeBits(r, k);
            }
        }
    }

    void decodeLossless(bitStream& bs, size_t blockSize, std::vector<sf::Int16>& decodedSamples, size_t offset) {
        if (blockSize == 0) return;

        uint32_t originalChecksum = bs.readBits(32);

        decodedSamples[offset] = static_cast<int16_t>(bs.readBits(16) - 32768);

        int k = 8;  // Default for fixed mode
        if (adaptive && blockSize > 1) {
            k = bs.readBits(4);
        }

        int16_t prevSample = decodedSamples[offset];
        int16_t prevDelta = 0;

        for (size_t i = 1; i < blockSize; i++) {
            uint32_t q = 0;
            while (bs.readBit()) {
                q++;
            }
            
            uint32_t r = (k > 0) ? bs.readBits(k) : 0;
            
            uint32_t uValue = (q << k) | r;
            
            int32_t delta;
            if (uValue & 1) {
                delta = -((uValue + 1) >> 1);
            } else {
                delta = (uValue >> 1);
            }
            
            int32_t predicted = prevSample + prevDelta;
            int32_t actual = predicted + delta;
            
            actual = std::clamp(actual, -32768, 32767);
            decodedSamples[offset + i] = static_cast<int16_t>(actual);
            
            prevDelta = actual - prevSample;
            prevSample = actual;
        }

        uint32_t decodedChecksum = calculateChecksum(&decodedSamples[offset], blockSize);
        if (decodedChecksum != originalChecksum) {
            throw std::runtime_error("Checksum verification failed - data corruption detected");
        }
    }

public:
    AudioCodec(int quantLevel = 64, bool isAdaptive = false, 
               bool isLossy = false, double bitrate = 0.0)
        : quantizationLevel(quantLevel)
        , adaptive(isAdaptive)
        , lossy(isLossy)
        , targetBitrate(bitrate)
        , coder(std::make_unique<golomb>(8, true)) {}

    AudioCodec(const AudioCodec& other)
        : quantizationLevel(other.quantizationLevel)
        , adaptive(other.adaptive)
        , lossy(other.lossy)
        , targetBitrate(other.targetBitrate)
        , buffer(other.buffer)
        , coder(std::make_unique<golomb>(*other.coder)) {}

    AudioCodec(AudioCodec&& other) noexcept = default;

    void encode(const std::string& inputFile, const std::string& outputFile) {
        if (!buffer.loadFromFile(inputFile)) {
            throw std::runtime_error("Failed to load input file");
        }

        const sf::Int16* samples = buffer.getSamples();
        const uint32_t sampleCount = buffer.getSampleCount();
        const uint8_t channelCount = buffer.getChannelCount();
        const uint32_t sampleRate = buffer.getSampleRate();

        bitStream bs;
        bs.openFile(outputFile, std::ios::binary | std::ios::out);

        bs.writeBits(channelCount, 8);
        bs.writeBits(sampleCount & 0xFFFF, 16);
        bs.writeBits(sampleCount >> 16, 16);
        bs.writeBits(sampleRate & 0xFFFF, 16);
        bs.writeBits(sampleRate >> 16, 16);
        bs.writeBits(quantizationLevel, 8);
        bs.writeBit(adaptive);
        bs.writeBit(lossy);

        for (uint32_t i = 0; i < sampleCount; i += BLOCK_SIZE) {
            const size_t blockSize = std::min(
                static_cast<size_t>(BLOCK_SIZE),
                static_cast<size_t>(sampleCount - i));
            
            bs.writeBits(blockSize, 16);

            if (lossy) {
                std::vector<double> blockData(blockSize);
                for (size_t j = 0; j < blockSize; j++) {
                    blockData[j] = samples[i + j] / 32767.0;
                }

                std::vector<double> dctCoeffs;
                dct(blockData, dctCoeffs);

                for (size_t j = 0; j < blockSize; j++) {
                    const double step = (1.0 / quantizationLevel) * (1.0 + j * j / (blockSize * blockSize));
                    const int16_t quantized = static_cast<int16_t>(std::round(dctCoeffs[j] / step));
                    coder->encode(quantized, bs);
                }
            } else {
                encodeLossless(samples + i, blockSize, bs);
            }
        }
        
        bs.flushBuffer();
    }

    void decode(const std::string& inputFile, const std::string& outputFile) {
        bitStream bs;
        bs.openFile(inputFile, std::ios::binary | std::ios::in);

        const uint8_t channelCount = bs.readBits(8);
        const uint32_t sampleCount = bs.readBits(16) | (bs.readBits(16) << 16);
        const uint32_t sampleRate = bs.readBits(16) | (bs.readBits(16) << 16);
        quantizationLevel = bs.readBits(8);
        adaptive = bs.readBit();
        lossy = bs.readBit();

        std::vector<sf::Int16> decodedSamples(sampleCount);
        size_t currentSample = 0;

        while (currentSample < sampleCount) {
            const size_t blockSize = std::min(
                bs.readBits(16),
                static_cast<size_t>(sampleCount - currentSample));

            if (lossy) {
                std::vector<double> dctCoeffs(blockSize);
                for (size_t j = 0; j < blockSize; j++) {
                    const double step = (1.0 / quantizationLevel) * (1.0 + j * j / (blockSize * blockSize));
                    const int16_t quantized = coder->decode(bs);
                    dctCoeffs[j] = quantized * step;
                }

                std::vector<double> output;
                idct(dctCoeffs, output);

                for (size_t j = 0; j < blockSize; j++) {
                    decodedSamples[currentSample + j] = static_cast<sf::Int16>(
                        std::clamp(std::round(output[j] * 32767.0), -32768.0, 32767.0));
                }
            } else {
                decodeLossless(bs, blockSize, decodedSamples, currentSample);
            }

            currentSample += blockSize;
        }

        if (!buffer.loadFromSamples(decodedSamples.data(), sampleCount, 
                                  channelCount, sampleRate)) {
            throw std::runtime_error("Failed to create output buffer");
        }

        if (!buffer.saveToFile(outputFile)) {
            throw std::runtime_error("Failed to save output file");
        }
    }
};
// int main(int argc, char const *argv[]) {
//     if (argc < 4) {
//         cout << "Usage: " << argv[0] << " <input.wav> <output.encoded> <output.decoded> [--lossy <target_bitrate>] [--adaptive]\n";
//         return 1;
//     }

//     string inputFile = argv[1];
//     string encodedFile = argv[2];
//     string decodedFile = argv[3];
//     bool isLossy = false;
//     double targetBitrate = 0;
//     bool isAdaptive = false;

//     for (int i = 4; i < argc; i++) {
//         if (string(argv[i]) == "--lossy" && i + 1 < argc) {
//             isLossy = true;
//             targetBitrate = stod(argv[++i]);
//         } else if (string(argv[i]) == "--adaptive") {
//             isAdaptive = true;
//         }
//     }

//     AudioCodec codec(4, isAdaptive, isLossy, targetBitrate);

//     try {
//         cout << "Encoding..." << endl;
//         codec.encode(inputFile, encodedFile);

//         cout << "Decoding..." << endl;
//         codec.decode(encodedFile, decodedFile);

//         cout << "Done!" << endl;
//     } catch (const exception& e) {
//         cerr << "Error: " << e.what() << endl;
//         return 1;
//     }

//     return 0;
// }