#ifndef VIDEOCODER_H
#define VIDEOCODER_H

#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <fstream>
#include <thread>
#include <chrono>
#include "../image/imageCoder.h"
#include "../util/bitStream.h"

using namespace std;
using namespace cv;
using namespace std::chrono;

struct EncodingMetrics {
    double psnr;
    double mse;
    double compressionRatio;
    double encodingTime;
    
    void calculatePSNR(const Mat& original, const Mat& reconstructed);
    void calculateCompressionRatio(size_t originalSize, size_t compressedSize);
};

struct VideoHeader {
    int width;
    int height;
    int blockSize;
    int iFrameInterval;
    int searchRange;
    double fps;
    int totalFrames;
    bool isLossy;
    int quantizationLevel;
    int targetBitrate;
};

class VideoCoder {
    friend class VideoTester;
private:
    ImageCoder imageCoder;
    int blockSize;
    int targetBitrate;
    bool isLossy;
    EncodingMetrics metrics;

    size_t totalBits = 0;
    unsigned char buffer = 0x00;
    int bufSize = 0;
    ofstream fs;

    Mat reconstructBlock(const Mat& predicted, const Mat& residual);
    void dct2D(Mat& block);
    void idct2D(Mat& block);
    vector<int> zigZagOrder(const Mat& block);
    Mat inverseZigZag(const vector<int>& coeffs);
    void quantizeBlock(Mat& block, bool isLuma);
    void dequantizeBlock(Mat& block, bool isLuma);
    int calculateOptimalM(const vector<int>& values);
    bool decideMode(const Mat& currentBlock, const Mat& predictedBlock, const Point2i& motionVector);
    void adjustQuantizationLevel(double currentBitrate);
    void roundMat(Mat& block);
    size_t getTotalBits() const;
    void writeBit(int bit);
    Mat calculateResidual(const Mat& current, const Mat& predicted);
    Point2i estimateMotion(const Mat& currentFrame, const Mat& referenceFrame, int blockX, int blockY, int blockSize, int searchRange);
    Mat decodeLossyResidual(bitStream& bs, const Size& blockSize);
    Mat decodeLosslessResidual(bitStream& bs, const Size& blockSize);
    int calculateIntraBitrate(const Mat& block);
    int calculateInterBitrate(const Mat& residual, const Point2i& motionVector);

public:
    int quantizationLevel;

    VideoCoder(int bs = 16, bool lossy = false, int qLevel = 1, int tBitrate = 0);
    void decodeIFrame(bitStream& bs, Mat& frame, const VideoHeader& header);
    void decodePFrame(bitStream& bs, Mat& frame, const Mat& prevFrame, const VideoHeader& header);
    EncodingMetrics encodeVideo(const string& inputPath, const string& outputPath, int iFrameInterval, int blockSize, int searchRange);
    void decodeVideo(const string& inputPath, const string& outputPath);
};

#endif // VIDEOCODER_H
