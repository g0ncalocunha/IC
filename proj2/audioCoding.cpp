#include <iostream>
#include <vector>
#include <cstdint>
#include <cmath>
#include <fstream>
#include <string>
#include <algorithm>
#include <SFML/Audio.hpp>
#include "golomb.cpp"
#include "bitStream.h"

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
    bitStream bs;

    void updateM(const vector<int>& residuals, int windowSize = 1000) {
        if (residuals.empty()) return;
        
        int start = max(0, static_cast<int>(residuals.size()) - windowSize);
        double sum = 0.0;
        
        for (int i = start; i < residuals.size(); i++) {
            sum += abs(residuals[i]);
        }
        
        double mean = sum / (residuals.size() - start);
        m = max(1, static_cast<int>(ceil(-1.0/log2(mean/(mean+1.0)))));
        
        golombEncoder.setM(m);
        golombDecoder.setM(m);
    }

    int16_t quantizeSample(int16_t sample) {
        if (!lossy || quantizationLevel <= 1) return sample;
        return (sample / quantizationLevel) * quantizationLevel;
    }

public:
    AudioCodec(int initialM = 10, bool isAdaptive = false, bool isLossy = false, double targetBitrate = 0.0) 
        : m(initialM), adaptive(isAdaptive), lossy(isLossy), 
          targetBitrate(targetBitrate), quantizationLevel(1),
          golombEncoder(initialM), golombDecoder(initialM) {}

    void encode(const string& inputFile, const string& outputFile) {
        if (!buffer.loadFromFile(inputFile)) {
            throw runtime_error("Failed to load: " + inputFile);
        }
        cerr << "Loaded input file successfully." << endl;

        bitStream encodeBs;
        encodeBs.openFile(outputFile, ios::out | ios::binary);
        cerr << "Opened output file successfully." << endl;

        const sf::Int16* samples = buffer.getSamples();
        const uint32_t sampleCount = buffer.getSampleCount();
        const uint8_t channelCount = buffer.getChannelCount();
        const uint32_t sampleRate = buffer.getSampleRate();

        cerr << "Audio parameters: " 
                << "Channels=" << static_cast<int>(channelCount) << ", "
                << "SampleCount=" << sampleCount << ", "
                << "SampleRate=" << sampleRate << endl;

        if (samples == nullptr || sampleCount == 0 || channelCount == 0 || sampleRate == 0) {
            throw runtime_error("Invalid audio file parameters!");
        }

        encodeBs.writeBits(channelCount, 8);
        encodeBs.writeBits(sampleCount, 32);
        encodeBs.writeBits(sampleRate, 32);
        encodeBs.writeBits(lossy ? 1 : 0, 1);
        encodeBs.writeBits(quantizationLevel, 8);
        encodeBs.flushBuffer();  // Flush after writing header

        cerr << "Header written to the output file." << endl;

        vector<vector<int>> residuals(channelCount);
        vector<int16_t> prevSamples(channelCount, 0);

        // Process samples
        for (size_t i = 0; i < sampleCount; i++) {
            uint8_t currentChannel = i % channelCount;
            int16_t currentSample = samples[i];
            int residual = currentSample - prevSamples[currentChannel];

            if (lossy) {
                residual = quantizeSample(residual);
            }

            golombEncoder.encode(residual, encodeBs);

            if (adaptive) {
                residuals[currentChannel].push_back(residual);
                if (residuals[currentChannel].size() >= 1000) {
                    updateM(residuals[currentChannel]);
                    residuals[currentChannel].clear();
                }
            }

            prevSamples[currentChannel] = currentSample;
        }

        encodeBs.flushBuffer();
        cerr << "Encoding completed successfully." << endl;
    }

    void decode(const string& inputFile, const string& outputFile) {
        bitStream decodeBs;
        try {
            decodeBs.openFile(inputFile, ios::in | ios::binary);
        } catch (const exception& e) {
            throw runtime_error("Failed to open encoded file: " + inputFile + " - " + e.what());
        }

        uint8_t channelCount;
        uint32_t totalSamples;
        uint32_t sampleRate;
        bool isLossyFile;
        int fileQuantizationLevel;

        try {
            channelCount = decodeBs.readBits(8);
            totalSamples = decodeBs.readBits(32);
            sampleRate = decodeBs.readBits(32);
            isLossyFile = decodeBs.readBits(1);
            fileQuantizationLevel = decodeBs.readBits(8);
        } catch (const exception& e) {
            throw runtime_error("Failed to read header: " + string(e.what()));
        }

        cerr << "Decoding parameters: Channels=" << (int)channelCount 
                  << ", Total samples=" << totalSamples 
                  << ", Sample rate=" << sampleRate << endl;

        if (channelCount == 0 || totalSamples == 0 || sampleRate == 0) {
            throw runtime_error("Invalid header data in encoded file");
        }

        vector<sf::Int16> decodedSamples;
        decodedSamples.reserve(totalSamples);
        vector<int16_t> prevSamples(channelCount, 0);
        vector<vector<int>> residuals(channelCount);

        try {
            for (uint32_t i = 0; i < totalSamples; i++) {
                uint8_t currentChannel = i % channelCount;
                int residual = golombDecoder.decode(decodeBs);
                int16_t reconstructedSample = prevSamples[currentChannel] + residual;
                
                decodedSamples.push_back(reconstructedSample);
                
                if (adaptive) {
                    residuals[currentChannel].push_back(residual);
                    if (residuals[currentChannel].size() >= 1000) {
                        updateM(residuals[currentChannel]);
                        residuals[currentChannel].clear();
                    }
                }

                prevSamples[currentChannel] = reconstructedSample;
            }
        } catch (const exception& e) {
            throw runtime_error("Error during sample decoding: " + string(e.what()));
        }

        cerr << "Decoded " << decodedSamples.size() << " samples." << endl;

        if (!buffer.loadFromSamples(decodedSamples.data(), decodedSamples.size(), 
                                  channelCount, sampleRate)) {
            throw runtime_error("Failed to create output buffer");
        }
        
        if (!buffer.saveToFile(outputFile)) {
            throw runtime_error("Failed to save: " + outputFile);
        }

        cerr << "Decoding completed successfully." << endl;
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