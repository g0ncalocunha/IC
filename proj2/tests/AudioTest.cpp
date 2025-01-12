#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include <vector>
#include <iomanip>
#include <filesystem>
#include "../audioCoding.cpp"
#include "../header/bitStream.h"
#include <SFML/Audio.hpp>

using namespace std;
namespace fs = std::filesystem;

void generateSampleAudio(const string& filename, int sampleRate, int durationInSeconds, int frequency) {
    sf::SoundBuffer buffer;
    vector<sf::Int16> samples;
    int totalSamples = sampleRate * durationInSeconds;

    for (int i = 0; i < totalSamples; i++) {
        float t = static_cast<float>(i) / sampleRate;
        samples.push_back(static_cast<sf::Int16>(sin(2 * M_PI * frequency * t) * 32767));
    }

    if (!buffer.loadFromSamples(samples.data(), samples.size(), 1, sampleRate)) {
        throw runtime_error("Failed to generate sample audio.");
    }

    if (!buffer.saveToFile(filename)) {
        throw runtime_error("Failed to save sample audio.");
    }
}

double calculateSNR(const vector<sf::Int16>& original, const vector<sf::Int16>& decoded) {
    double signalPower = 0.0;
    double noisePower = 0.0;

    for (size_t i = 0; i < original.size(); i++) {
        double signal = original[i];
        double noise = original[i] - decoded[i];
        signalPower += signal * signal;
        noisePower += noise * noise;
    }

    if (noisePower == 0) return INFINITY; // Perfect reconstruction
    return 10.0 * log10(signalPower / noisePower);
}

double estimateSubjectiveQuality(double snr) {
    if (snr >= 30.0) return 5.0; // Imperceptible
    if (snr >= 20.0) return 4.0; // Perceptible, not annoying
    if (snr >= 10.0) return 3.0; // Slightly annoying
    if (snr >= 5.0) return 2.0;  // Annoying
    return 1.0;                  // Very annoying
}

void testCodec(bool lossy, bool adaptive, double targetBitrate = 0.0) {
    string inputFile = "audios/sample.wav";
    string encodedFile = "audios/results/encoded/sample02.encoded";
    string decodedFile = "audios/results/output.wav";
    string resultFile = "audios/results/test_results.txt";

    fs::create_directories("audios/results/encoded");

    generateSampleAudio(inputFile, 44100, 2, 440);

    AudioCodec codec(4, adaptive, lossy, targetBitrate);

    ofstream resultStream(resultFile, ios::app);
    if (!resultStream.is_open()) {
        throw runtime_error("Failed to open result file: " + resultFile);
    }

    try {
        cout << "Testing encoding..." << endl;
        codec.encode(inputFile, encodedFile);

        cout << "Testing decoding..." << endl;
        codec.decode(encodedFile, decodedFile);

        sf::SoundBuffer originalBuffer, decodedBuffer;
        if (!originalBuffer.loadFromFile(inputFile)) {
            throw runtime_error("Failed to load original audio.");
        }
        if (!decodedBuffer.loadFromFile(decodedFile)) {
            throw runtime_error("Failed to load decoded audio.");
        }

        const sf::Int16* originalSamples = originalBuffer.getSamples();
        const sf::Int16* decodedSamples = decodedBuffer.getSamples();
        size_t originalSampleCount = originalBuffer.getSampleCount();
        size_t decodedSampleCount = decodedBuffer.getSampleCount();
        size_t sampleCount = min(originalSampleCount, decodedSampleCount);

        assert(sampleCount > 0 && "Sample count must be greater than 0!");

        vector<sf::Int16> originalCopy(originalSamples, originalSamples + sampleCount);
        vector<sf::Int16> decodedCopy(decodedSamples, decodedSamples + sampleCount);

        // Calculate SNR
        double snr = calculateSNR(originalCopy, decodedCopy);
        double subjectiveQuality = estimateSubjectiveQuality(snr);

        // Check lossless status
        bool isLossless = (snr == INFINITY);
        size_t mismatchedSamples = 0;
        for (size_t i = 0; i < sampleCount; i++) {
            if (originalCopy[i] != decodedCopy[i]) {
                mismatchedSamples++;
            }
        }

        // Calculate compression ratio
        size_t originalSize = fs::file_size(inputFile);
        size_t encodedSize = fs::file_size(encodedFile);
        double compressionRatio = static_cast<double>(originalSize) / encodedSize;

        ostringstream results;
        results << "\n=== Audio Compression Test Results ===\n";
        results << "Lossless: " << (isLossless ? "Yes" : "No") << "\n\n";
        results << "Objective Measures:\n";
        results << "Total samples: " << sampleCount << "\n";
        results << "Mismatched samples: " << mismatchedSamples << " ("
                << fixed << setprecision(2) << (100.0 * mismatchedSamples / sampleCount) << "%)\n";
        results << "Signal-to-Noise Ratio: " << fixed << setprecision(2) << snr << " dB\n\n";
        results << "Subjective Quality (ITU-R Scale):\n";
        results << "Estimated Quality: " << fixed << setprecision(1) << subjectiveQuality << " "
                << (subjectiveQuality >= 4.0 ? "Imperceptible"
                    : subjectiveQuality >= 3.0 ? "Slightly annoying"
                    : subjectiveQuality >= 2.0 ? "Annoying"
                    : "Very annoying") << "\n\n";
        results << "Compression Statistics:\n";
        results << "Compression ratio: " << fixed << setprecision(2) << compressionRatio << ":1\n";
        results << "Space saving: " << ((1.0 - 1.0/compressionRatio) * 100) << "%\n";
        results << "======================================\n";

        cout << results.str();

    } catch (const exception& e) {
        cerr << "Test failed: " << e.what() << endl;
        resultStream.close();
        throw;
    }

    resultStream.close();
}

int main() {
    cout << "Running lossless adaptive test..." << endl;
    testCodec(false, true);

    cout << "Running lossless fixed test..." << endl;
    testCodec(false, false);

    cout << "Running lossy adaptive test..." << endl;
    testCodec(true, true, 64.0);

    cout << "Running lossy fixed test..." << endl;
    testCodec(true, false, 64.0);

    cout << "All tests passed successfully!" << endl;
    return 0;
}
