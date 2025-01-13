#ifndef AUDIOCODING_H
#define AUDIOCODING_H

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

    int calculateOptimalM(const std::vector<int16_t>& deltas);
    void dct(const std::vector<double>& input, std::vector<double>& output);
    void idct(const std::vector<double>& input, std::vector<double>& output);
    void encodeLossless(const sf::Int16* samples, size_t blockSize, bitStream& bs);
    void decodeLossless(bitStream& bs, size_t blockSize, std::vector<sf::Int16>& decodedSamples, size_t offset);

public:
    AudioCodec(int quantLevel = 64, bool isAdaptive = false, bool isLossy = false, double bitrate = 0.0);
    AudioCodec(const AudioCodec& other);
    AudioCodec(AudioCodec&& other) noexcept;

    void encode(const std::string& inputFile, const std::string& outputFile);
    void decode(const std::string& inputFile, const std::string& outputFile);
};

#endif // AUDIOCODING_H