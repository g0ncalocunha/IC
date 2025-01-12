#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <fstream>
#include <iomanip>
#include "../videoCoder.cpp"

using namespace std;
using namespace std::chrono;

class VideoTestConfiguration {
public:
    struct EncodingParams {
        int iFrameInterval;
        int blockSize;
        int searchRange;
        bool isLossy;
        int quantizationLevel;
        int targetBitrate;
    };

    struct TestCase {
        string name;
        string inputPath;
        string encodedPath;
        string outputPath;
        EncodingParams params;
        size_t frameCount;
        size_t originalSize;
    };

private:
    vector<TestCase> testCases;

    size_t getFileSize(const string& path) {
        ifstream file(path, ios::binary | ios::ate);
        if (!file.is_open()) {
            throw runtime_error("Cannot open file: " + path);
        }
        return file.tellg();
    }

    size_t getFrameCount(const string& path) {
        VideoCapture cap(path);
        if (!cap.isOpened()) {
            throw runtime_error("Cannot open video: " + path);
        }
        return static_cast<size_t>(cap.get(CAP_PROP_FRAME_COUNT));
    }

public:
    void addTestCase(const string& name, const string& input, const EncodingParams& params) {
        TestCase testCase;
        testCase.name = name;
        testCase.inputPath = input;
        testCase.encodedPath = "encoded_" + name + ".bin";
        testCase.outputPath = "decoded_" + name + ".mp4";
        testCase.params = params;
        
        // Get frame count and file size
        testCase.frameCount = getFrameCount(input);
        testCase.originalSize = getFileSize(input);
        
        testCases.push_back(testCase);
    }

    const vector<TestCase>& getTestCases() const {
        return testCases;
    }
};

class VideoMetricsCalculator {
public:
    static double calculateFramePSNR(const Mat& original, const Mat& decoded) {
        if (original.empty() || decoded.empty()) {
            return 0.0;
        }

        Mat diff;
        absdiff(original, decoded, diff);
        diff.convertTo(diff, CV_64F);
        diff = diff.mul(diff);
        
        double mse = sum(diff)[0] / (diff.total() * original.channels());
        return (mse > 0) ? 10.0 * log10(255.0 * 255.0 / mse) : 100.0;
    }

    static double calculateAveragePSNR(const string& originalPath, const string& decodedPath) {
        VideoCapture origVideo(originalPath);
        VideoCapture decodedVideo(decodedPath);
        
        if (!origVideo.isOpened() || !decodedVideo.isOpened()) {
            throw runtime_error("Cannot open video files for PSNR calculation");
        }

        double totalPSNR = 0.0;
        int frameCount = 0;
        Mat origFrame, decodedFrame;

        while (origVideo.read(origFrame) && decodedVideo.read(decodedFrame)) {
            if (origFrame.empty() || decodedFrame.empty()) break;
            
            double framePSNR = calculateFramePSNR(origFrame, decodedFrame);
            totalPSNR += framePSNR;
            frameCount++;

            if (frameCount % 10 == 0) {
                cout << "\rCalculating PSNR: " << frameCount << " frames processed" << flush;
            }
        }

        cout << endl;
        return frameCount > 0 ? totalPSNR / frameCount : 0.0;
    }
};

class VideoTester {
private:
    VideoTestConfiguration config;

    void printTestInfo(const VideoTestConfiguration::TestCase& testCase) {
        cout << "\n" << string(50, '=') << endl;
        cout << "Starting test for video: " << testCase.name << endl;
        cout << "Frame count: " << testCase.frameCount << endl;
        cout << "Original size: " << testCase.originalSize << " bytes" << endl;
        cout << "Configuration:" << endl;
        cout << "  I-Frame Interval: " << testCase.params.iFrameInterval << endl;
        cout << "  Block Size: " << testCase.params.blockSize << endl;
        cout << "  Search Range: " << testCase.params.searchRange << endl;
        cout << string(50, '=') << endl;
    }

    void printMetrics(const EncodingMetrics& metrics, 
                     double avgPSNR,
                     size_t compressedSize,
                     size_t originalSize,
                     size_t frameCount) {
        double fps = frameCount / metrics.encodingTime;
        double compressionRatio = static_cast<double>(originalSize) / compressedSize;

        cout << "\nEncoding Metrics:" << endl;
        cout << fixed << setprecision(2);
        cout << "  Average PSNR: " << avgPSNR << " dB" << endl;
        cout << "  Encoding Time: " << metrics.encodingTime << " seconds" << endl;
        cout << "  Frames Per Second: " << fps << endl;
        cout << "  Compressed Size: " << compressedSize << " bytes" << endl;
        cout << "  Compression Ratio: " << compressionRatio << ":1" << endl;
    }

public:
    explicit VideoTester(const VideoTestConfiguration& testConfig) : config(testConfig) {}

    void runAllTests() {
        for (const auto& testCase : config.getTestCases()) {
            try {
                runSingleTest(testCase);
            } catch (const exception& e) {
                cerr << "Error testing " << testCase.name << ": " << e.what() << endl;
            }
        }
    }

    void runSingleTest(const VideoTestConfiguration::TestCase& testCase) {
        printTestInfo(testCase);

        VideoCoder coder(testCase.params.blockSize,
                        testCase.params.isLossy,
                        testCase.params.quantizationLevel,
                        testCase.params.targetBitrate);

        cout << "Encoding video..." << endl;
        auto metrics = coder.encodeVideo(testCase.inputPath,
                                       testCase.encodedPath,
                                       testCase.params.iFrameInterval,
                                       testCase.params.blockSize,
                                       testCase.params.searchRange);
        cout << "Encoding completed!" << endl;

        cout << "Decoding video..." << endl;
        coder.decodeVideo(testCase.encodedPath, testCase.outputPath);
        cout << "Decoding completed!" << endl;

        cout << "Calculating final metrics..." << endl;
        double avgPSNR = VideoMetricsCalculator::calculateAveragePSNR(
            testCase.inputPath, testCase.outputPath);
        size_t compressedSize = getFileSize(testCase.encodedPath);

        printMetrics(metrics, avgPSNR, compressedSize, 
                    testCase.originalSize, testCase.frameCount);
    }

private:
    size_t getFileSize(const string& path) {
        ifstream file(path, ios::binary | ios::ate);
        return file.tellg();
    }
};

int main() {
    try {
        VideoTestConfiguration config;

        config.addTestCase("akiyo_cif", 
            "../videos/akiyo_cif.y4m", 
            {10, 16, 16, true, 1, 0});

        config.addTestCase("bus_cif",
            "../videos/bus_cif.y4m",
            {15, 16, 16, true, 5, 500000});

        config.addTestCase("deadline_cif",
            "../videos/deadline_cif.y4m",
            {10, 8, 8, true, 3, 300000});

        VideoTester tester(config);
        tester.runAllTests();

    } catch (const exception& e) {
        cerr << "Fatal error: " << e.what() << endl;
        return 1;
    }

    return 0;
}