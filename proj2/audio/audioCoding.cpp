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
#include "../util/golomb.cpp"

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

    int calculateOptimalM(const std::vector<int16_t>& deltas) {
        if (deltas.empty()) return 8;
        
        // Calculate mean absolute value
        double sum = 0;
        for (const auto& delta : deltas) {
            sum += std::abs(delta);
        }
        double mean = sum / deltas.size();
        
        // Choose M based on mean value
        if (mean < 4) return 4;
        if (mean < 8) return 8;
        if (mean < 16) return 16;
        return 32;
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
        
        // Write first sample directly
        bs.writeBits(static_cast<uint16_t>(samples[0] + 32768), 16);
        
        // Calculate deltas
        std::vector<int16_t> deltas(blockSize - 1);
        for (size_t i = 1; i < blockSize; i++) {
            deltas[i - 1] = samples[i] - samples[i - 1];
        }
        
        // Update Golomb parameter if needed
        if (adaptive && !deltas.empty()) {
            int m = calculateOptimalM(deltas);
            coder->setM(m);
            bs.writeBits(m, 6);
        }
        
        // Encode deltas with Golomb coding
        for (const auto& delta : deltas) {
            coder->encode(delta, bs);
        }
    }

    void decodeLossless(bitStream& bs, size_t blockSize, std::vector<sf::Int16>& decodedSamples, size_t offset) {
        if (blockSize == 0) return;
        
        // Read first sample directly
        decodedSamples[offset] = static_cast<int16_t>(bs.readBits(16) - 32768);
        
        // Read adaptive parameter if needed
        if (adaptive && blockSize > 1) {
            int m = bs.readBits(6);
            coder->setM(m);
        }
        
        // Decode remaining samples using deltas
        for (size_t i = 1; i < blockSize; i++) {
            int16_t delta = coder->decode(bs);
            decodedSamples[offset + i] = decodedSamples[offset + i - 1] + delta;
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

        // Write header
        bs.writeBits(channelCount, 8);
        bs.writeBits(sampleCount & 0xFFFF, 16);
        bs.writeBits(sampleCount >> 16, 16);
        bs.writeBits(sampleRate & 0xFFFF, 16);
        bs.writeBits(sampleRate >> 16, 16);
        bs.writeBits(quantizationLevel, 8);
        bs.writeBit(adaptive);
        bs.writeBit(lossy);

        // Process blocks
        for (uint32_t i = 0; i < sampleCount; i += BLOCK_SIZE) {
            const size_t blockSize = std::min(
                static_cast<size_t>(BLOCK_SIZE),
                static_cast<size_t>(sampleCount - i));

            bs.writeBits(blockSize, 16);

            if (lossy) {
                // Convert to doubles [-1,1]
                std::vector<double> blockData(blockSize);
                for (size_t j = 0; j < blockSize; j++) {
                    blockData[j] = samples[i + j] / 32767.0;
                }

                std::vector<double> dctCoeffs;
                dct(blockData, dctCoeffs);

                // Quantize and encode DCT coefficients
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

        // Read header
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
                // Decode and dequantize DCT coefficients
                std::vector<double> dctCoeffs(blockSize);
                for (size_t j = 0; j < blockSize; j++) {
                    const double step = (1.0 / quantizationLevel) * (1.0 + j * j / (blockSize * blockSize));
                    const int16_t quantized = coder->decode(bs);
                    dctCoeffs[j] = quantized * step;
                }

                // Apply IDCT
                std::vector<double> output;
                idct(dctCoeffs, output);

                // Convert back to samples
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
