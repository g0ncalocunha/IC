#include <iostream>
#include <vector>
#include <cstdint>
#include <cmath>
#include <fstream>
#include <string>
#include <algorithm>
#include <SFML/Audio.hpp>
#include "golomb.cpp"
#include "header/bitStream.h"

using namespace std;

class AudioCodec {
private:
    int m;
    bool adaptive;
    bool lossy;
    double targetBitrate;
    int quantizationLevel;
    golomb golombEncoder;
    golomb golombDecoder;
    sf::SoundBuffer buffer;

    int16_t clampSample(int value) {
        if (value > 32767) return 32767;
        if (value < -32768) return -32768;
        return static_cast<int16_t>(value);
    }

    void updateM(const vector<int>& residuals) {
        if (residuals.empty()) return;
        
        double sum = 0.0;
        int count = 0;
        
        for (int residual : residuals) {
            sum += abs(residual);
            count++;
        }
        
        if (count == 0) return;
        
        double mean = sum / count;
        
        // Ensure M stays within reasonable bounds for Golomb coding
        if (mean > 0) {
            int newM = max(2, min(8, static_cast<int>(round(log2(mean + 1)))));
            if (abs(newM - m) > 1) {
                m = newM;
                golombEncoder.setM(m);
                golombDecoder.setM(m);
            }
        }
    }

public:
    AudioCodec(int initialM = 4, bool isAdaptive = false, bool isLossy = false, double targetBitrate = 0.0) 
        : m(initialM), adaptive(isAdaptive), lossy(isLossy), 
          targetBitrate(targetBitrate), quantizationLevel(1),
          golombEncoder(initialM), golombDecoder(initialM) {}

    void encode(const string& inputFile, const string& outputFile) {
        if (!buffer.loadFromFile(inputFile)) {
            throw runtime_error("Failed to load: " + inputFile);
        }

        bitStream encodeBs;
        encodeBs.openFile(outputFile, ios::out | ios::binary);

        const sf::Int16* samples = buffer.getSamples();
        const uint32_t sampleCount = buffer.getSampleCount();
        const uint8_t channelCount = buffer.getChannelCount();
        const uint32_t sampleRate = buffer.getSampleRate();

        // Write header with 32-bit chunks
        encodeBs.writeBits(channelCount, 8);
        encodeBs.writeBits((sampleCount >> 16) & 0xFFFF, 16);
        encodeBs.writeBits(sampleCount & 0xFFFF, 16);
        encodeBs.writeBits((sampleRate >> 16) & 0xFFFF, 16);
        encodeBs.writeBits(sampleRate & 0xFFFF, 16);
        encodeBs.writeBits(adaptive ? 1 : 0, 1);
        encodeBs.writeBits(m, 4);  // M is now bounded to 2-8
        encodeBs.flushBuffer();

        vector<int16_t> prevSamples(channelCount, 0);
        vector<int> recentResiduals;
        const int WINDOW_SIZE = 16;  // Smaller window for stability

        for (uint32_t i = 0; i < sampleCount; i++) {
            uint8_t currentChannel = i % channelCount;
            int16_t currentSample = samples[i];
            
            // Calculate residual
            int residual = currentSample - prevSamples[currentChannel];
            
            if (adaptive) {
                recentResiduals.push_back(residual);
                if (recentResiduals.size() >= WINDOW_SIZE) {
                    int oldM = m;
                    updateM(recentResiduals);
                    
                    if (m != oldM) {
                        encodeBs.writeBits(1, 1);
                        encodeBs.writeBits(m, 4);
                    } else {
                        encodeBs.writeBits(0, 1);
                    }
                    
                    recentResiduals.clear();
                }
            }

            golombEncoder.encode(residual, encodeBs);
            prevSamples[currentChannel] = currentSample;
        }

        encodeBs.flushBuffer();
    }

    void decode(const string& inputFile, const string& outputFile) {
        bitStream decodeBs;
        decodeBs.openFile(inputFile, ios::in | ios::binary);

        // Read header in chunks
        uint8_t channelCount = decodeBs.readBits(8);
        uint32_t sampleCount = (decodeBs.readBits(16) << 16) | decodeBs.readBits(16);
        uint32_t sampleRate = (decodeBs.readBits(16) << 16) | decodeBs.readBits(16);
        bool isAdaptive = decodeBs.readBits(1);
        m = decodeBs.readBits(4);
        golombDecoder.setM(m);

        vector<sf::Int16> decodedSamples;
        decodedSamples.reserve(sampleCount);
        vector<int16_t> prevSamples(channelCount, 0);
        vector<int> recentResiduals;
        const int WINDOW_SIZE = 16;

        for (uint32_t i = 0; i < sampleCount; i++) {
            uint8_t currentChannel = i % channelCount;
            
            if (isAdaptive && i > 0 && i % WINDOW_SIZE == 0) {
                bool mChanged = decodeBs.readBits(1);
                if (mChanged) {
                    m = decodeBs.readBits(4);
                    golombDecoder.setM(m);
                }
            }

            int residual = golombDecoder.decode(decodeBs);
            int16_t reconstructedSample = clampSample(prevSamples[currentChannel] + residual);
            decodedSamples.push_back(reconstructedSample);
            
            prevSamples[currentChannel] = reconstructedSample;
            
            if (isAdaptive) {
                recentResiduals.push_back(residual);
                if (recentResiduals.size() >= WINDOW_SIZE) {
                    recentResiduals.clear();
                }
            }
        }

        if (!buffer.loadFromSamples(decodedSamples.data(), decodedSamples.size(), 
                                  channelCount, sampleRate)) {
            throw runtime_error("Failed to create output buffer");
        }
        
        if (!buffer.saveToFile(outputFile)) {
            throw runtime_error("Failed to save: " + outputFile);
        }
    }
};

int main(int argc, char const *argv[]) {
    if (argc < 4) {
        cout << "Usage: " << argv[0] << " <input.wav> <output.encoded> <output.decoded> "
                  << "[--lossy <target_bitrate>] [--adaptive]" << endl;
        return 1;
    }

    string inputFile = argv[1];
    string encodedFile = argv[2];
    string decodedFile = argv[3];
    bool isLossy = false;
    double targetBitrate = 0;
    bool isAdaptive = false;

    for (int i = 4; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--lossy" && i + 1 < argc) {
            isLossy = true;
            targetBitrate = stod(argv[++i]);
        } else if (arg == "--adaptive") {
            isAdaptive = true;
        }
    }

    AudioCodec codec(10, isAdaptive, isLossy, targetBitrate);
    
    try {
        cout << "Encoding..." << endl;
        codec.encode(inputFile, encodedFile);
        
        cout << "Decoding..." << endl;
        codec.decode(encodedFile, decodedFile);
        
        cout << "Done!" << endl;
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}