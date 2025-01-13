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
    
    void calculatePSNR(const Mat& original, const Mat& reconstructed) {
        Mat diff;
        absdiff(original, reconstructed, diff);
        diff.convertTo(diff, CV_32F);
        diff = diff.mul(diff);
        mse = sum(diff)[0] / (diff.total() * 255.0 * 255.0);
        psnr = 10.0 * log10(1.0 / mse);
    }

    void calculateCompressionRatio(size_t originalSize, size_t compressedSize) {
        compressionRatio = static_cast<double>(originalSize) / compressedSize;
    }
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

    Mat reconstructBlock(const Mat& predicted, const Mat& residual) {
        Mat reconstructed;
        add(predicted, residual, reconstructed, noArray(), CV_8UC3);
        return reconstructed;
    }
    
    void dct2D(Mat& block) {
        Mat floatBlock;
        block.convertTo(floatBlock, CV_32F);
        dct(floatBlock, floatBlock);
        floatBlock.copyTo(block);
    }

    void idct2D(Mat& block) {
        Mat floatBlock;
        block.convertTo(floatBlock, CV_32F);
        idct(floatBlock, floatBlock);
        floatBlock.copyTo(block);
    }

    vector<int> zigZagOrder(const Mat& block) {
        static const int zigZagPattern[64] = {
            0,  1,  5,  6,  14, 15, 27, 28,
            2,  4,  7,  13, 16, 26, 29, 42,
            3,  8,  12, 17, 25, 30, 41, 43,
            9,  11, 18, 24, 31, 40, 44, 53,
            10, 19, 23, 32, 39, 45, 52, 54,
            20, 22, 33, 38, 46, 51, 55, 60,
            21, 34, 37, 47, 50, 56, 59, 61,
            35, 36, 48, 49, 57, 58, 62, 63
        };
        
        vector<int> result;
        result.reserve(64);
        
        for (int i = 0; i < 64; i++) {
            int row = zigZagPattern[i] / 8;
            int col = zigZagPattern[i] % 8;
            result.push_back(block.at<float>(row, col));
        }
        return result;
    }

    Mat inverseZigZag(const vector<int>& coeffs) {
        Mat block = Mat::zeros(8, 8, CV_32F);
        static const int zigZagPattern[64] = {
            0,  1,  5,  6,  14, 15, 27, 28,
            2,  4,  7,  13, 16, 26, 29, 42,
            3,  8,  12, 17, 25, 30, 41, 43,
            9,  11, 18, 24, 31, 40, 44, 53,
            10, 19, 23, 32, 39, 45, 52, 54,
            20, 22, 33, 38, 46, 51, 55, 60,
            21, 34, 37, 47, 50, 56, 59, 61,
            35, 36, 48, 49, 57, 58, 62, 63
        };
        
        for (int i = 0; i < 64; i++) {
            int row = zigZagPattern[i] / 8;
            int col = zigZagPattern[i] % 8;
            block.at<float>(row, col) = coeffs[i];
        }
        return block;
    }

    void quantizeBlock(Mat& block, bool isLuma) {
        static const Mat lumaMatrix = (Mat_<float>(8, 8) <<
            16, 11, 10, 16, 24, 40, 51, 61,
            12, 12, 14, 19, 26, 58, 60, 55,
            14, 13, 16, 24, 40, 57, 69, 56,
            14, 17, 22, 29, 51, 87, 80, 62,
            18, 22, 37, 56, 68, 109, 103, 77,
            24, 35, 55, 64, 81, 104, 113, 92,
            49, 64, 78, 87, 103, 121, 120, 101,
            72, 92, 95, 98, 112, 100, 103, 99);
            
        static const Mat chromaMatrix = (Mat_<float>(8, 8) <<
            17, 18, 24, 47, 99, 99, 99, 99,
            18, 21, 26, 66, 99, 99, 99, 99,
            24, 26, 56, 99, 99, 99, 99, 99,
            47, 66, 99, 99, 99, 99, 99, 99,
            99, 99, 99, 99, 99, 99, 99, 99,
            99, 99, 99, 99, 99, 99, 99, 99,
            99, 99, 99, 99, 99, 99, 99, 99,
            99, 99, 99, 99, 99, 99, 99, 99);
            
        const Mat& qMatrix = isLuma ? lumaMatrix : chromaMatrix;
        divide(block, qMatrix * quantizationLevel, block);
        roundMat(block);
    }

    void dequantizeBlock(Mat& block, bool isLuma) {
        static const Mat lumaMatrix = (Mat_<float>(8, 8) <<
            16, 11, 10, 16, 24, 40, 51, 61,
            12, 12, 14, 19, 26, 58, 60, 55,
            14, 13, 16, 24, 40, 57, 69, 56,
            14, 17, 22, 29, 51, 87, 80, 62,
            18, 22, 37, 56, 68, 109, 103, 77,
            24, 35, 55, 64, 81, 104, 113, 92,
            49, 64, 78, 87, 103, 121, 120, 101,
            72, 92, 95, 98, 112, 100, 103, 99);
            
        static const Mat chromaMatrix = (Mat_<float>(8, 8) <<
            17, 18, 24, 47, 99, 99, 99, 99,
            18, 21, 26, 66, 99, 99, 99, 99,
            24, 26, 56, 99, 99, 99, 99, 99,
            47, 66, 99, 99, 99, 99, 99, 99,
            99, 99, 99, 99, 99, 99, 99, 99,
            99, 99, 99, 99, 99, 99, 99, 99,
            99, 99, 99, 99, 99, 99, 99, 99,
            99, 99, 99, 99, 99, 99, 99, 99);
            
        const Mat& qMatrix = isLuma ? lumaMatrix : chromaMatrix;
        multiply(block, qMatrix * quantizationLevel, block);
    }

    int calculateOptimalM(const vector<int>& values) {
        double mean = 0.0;
        for (int val : values) {
            mean += val;
        }
        mean /= values.size();
        return max(1, (int)round(-1.0 / log2(mean / (mean + 1.0))));
    }

    bool decideMode(const Mat& currentBlock, const Mat& predictedBlock, 
                    const Point2i& motionVector) {
        int intraBits = calculateIntraBitrate(currentBlock);
        Mat residual = calculateResidual(currentBlock, predictedBlock);
        int interBits = calculateInterBitrate(residual, motionVector);
        return intraBits < interBits;
    }

    void adjustQuantizationLevel(double currentBitrate) {
        if (currentBitrate > targetBitrate) {
            quantizationLevel = min(51, quantizationLevel + 1);
        } else if (currentBitrate < targetBitrate * 0.9) {
            quantizationLevel = max(1, quantizationLevel - 1);
        }
    }

    void roundMat(Mat& block) {
        block.forEach<float>([](float& pixel, const int* position) -> void {
            pixel = std::round(pixel);
        });
    }

    size_t getTotalBits() const { return totalBits; }
    void writeBit(int bit) {
        buffer = buffer | (bit << (7 - (bufSize % 8)));
        bufSize++;
        totalBits++;
        if (bufSize == 8) {
            fs.put(buffer);
            buffer = 0x00;
            bufSize = 0;
        }
    }
    
    Mat calculateResidual(const Mat& current, const Mat& predicted) {
        Mat residual;
        subtract(current, predicted, residual, noArray(), CV_16SC3);
        return residual;
    }

    Point2i estimateMotion(const Mat& currentFrame, const Mat& referenceFrame, 
                        int blockX, int blockY, int blockSize, int searchRange) {
        Point2i bestMotion(0, 0);
        double minSAD = numeric_limits<double>::max();
        
        Rect currentBlock(blockX, blockY, 
                         min(blockSize, currentFrame.cols - blockX),
                         min(blockSize, currentFrame.rows - blockY));
        
        for(int dy = -searchRange; dy <= searchRange; dy++) {
            for(int dx = -searchRange; dx <= searchRange; dx++) {
                if(blockY + dy < 0 || blockX + dx < 0 || 
                   blockY + blockSize + dy > referenceFrame.rows ||
                   blockX + blockSize + dx > referenceFrame.cols) {
                    continue;
                }
                
                Rect searchBlock(blockX + dx, blockY + dy, 
                               currentBlock.width, currentBlock.height);
                Mat curr = currentFrame(currentBlock);
                Mat ref = referenceFrame(searchBlock);
                
                Mat diff = calculateResidual(curr, ref);
                double SAD = sum(abs(diff))[0];
                
                if(SAD < minSAD) {
                    minSAD = SAD;
                    bestMotion = Point2i(dx, dy);
                }
            }
        }
        return bestMotion;
    }

    Mat decodeLossyResidual(bitStream& bs, const Size& blockSize) {
        Mat residual(blockSize, CV_16SC3);
        vector<Mat> channels(3);
        for (int c = 0; c < 3; c++) {
            channels[c] = Mat(blockSize, CV_16SC1);
            for (int i = 0; i < blockSize.height; i++) {
                for (int j = 0; j < blockSize.width; j++) {
                    channels[c].at<short>(i,j) = bs.readBits(8) - 128;
                }
            }
        }
        merge(channels, residual);
        return residual;
    }
    
    Mat decodeLosslessResidual(bitStream& bs, const Size& blockSize) {
        return decodeLossyResidual(bs, blockSize); 
    }

    int calculateIntraBitrate(const Mat& block) {
        auto residuals = imageCoder.calculateResiduals(block, Predictors::JPEG_LS);
        return residuals[0].size() * 8;
    }

    int calculateInterBitrate(const Mat& residual, const Point2i& motionVector) {
        return (residual.total() * residual.channels() * 8) + (2 * 16);
    }

public:
    int quantizationLevel;

     VideoCoder(int bs = 16, bool lossy = false, int qLevel = 1, int tBitrate = 0) 
        : blockSize(bs), isLossy(lossy), quantizationLevel(qLevel), 
          targetBitrate(tBitrate), imageCoder() {}

    void decodeIFrame(bitStream& bs, Mat& frame, const VideoHeader& header) {
    frame = Mat(header.height, header.width, CV_8UC3, Scalar(0, 0, 0));

    if (header.isLossy) {
        for (int y = 0; y < frame.rows; y += 8) {
            for (int x = 0; x < frame.cols; x += 8) {
                for (int c = 0; c < 3; c++) {
                    Mat block(8, 8, CV_32F);
                    vector<int> zigZag(64);
                    for (int i = 0; i < 64; i++) {
                        zigZag[i] = bs.readBits(8) - 128;
                    }
                    block = inverseZigZag(zigZag);
                    dequantizeBlock(block, c == 0);
                    idct2D(block);
                    block.convertTo(block, CV_8U);

                    Rect blockRect(x, y, block.cols, block.rows);
                    Mat roi = frame(blockRect).colRange(0, block.cols).rowRange(0, block.rows);
                    roi.forEach<Vec3b>([&block, c](Vec3b& pixel, const int* position) {
                        pixel[c] = saturate_cast<uchar>(block.at<float>(position[0], position[1]));
                    });
                }
            }
        }
    } else {
        // Decode lossless I-frame
        vector<Mat> channels(3);
        for (int c = 0; c < 3; c++) {
            channels[c] = Mat(frame.rows, frame.cols, CV_8UC1);
            for (int i = 0; i < frame.rows; i++) {
                for (int j = 0; j < frame.cols; j++) {
                    channels[c].at<uchar>(i, j) = bs.readBits(8);
                }
            }
        }
        merge(channels, frame);
    }
}

    void decodePFrame(bitStream& bs, Mat& frame, const Mat& prevFrame, const VideoHeader& header) {
        frame = Mat(header.height, header.width, CV_8UC3, Scalar(0, 0, 0));

        for (int y = 0; y < frame.rows; y += header.blockSize) {
            for (int x = 0; x < frame.cols; x += header.blockSize) {
                int mvX = bs.readBits(8) - 128;
                int mvY = bs.readBits(8) - 128;

                Rect currBlock(x, y,
                    min(header.blockSize, frame.cols - x),
                    min(header.blockSize, frame.rows - y));

                int predX = max(0, min(frame.cols - currBlock.width, x + mvX));
                int predY = max(0, min(frame.rows - currBlock.height, y + mvY));

                Rect refBlock(predX, predY, currBlock.width, currBlock.height);
                Mat predictedBlock = prevFrame(refBlock);

                Mat residual = header.isLossy ? decodeLossyResidual(bs, currBlock.size())
                                            : decodeLosslessResidual(bs, currBlock.size());

                residual.convertTo(residual, predictedBlock.type());
                Mat reconstructed;
                add(predictedBlock, residual, reconstructed, noArray(), CV_8UC3);
                reconstructed.copyTo(frame(currBlock));
            }
        }
    }


    EncodingMetrics encodeVideo(const string& inputPath, const string& outputPath, 
                               int iFrameInterval, int blockSize, int searchRange) {
        auto startTime = high_resolution_clock::now();
        
        VideoCapture cap(inputPath);
        if (!cap.isOpened()) throw runtime_error("Cannot open input video");

        VideoHeader header = {
            static_cast<int>(cap.get(CAP_PROP_FRAME_WIDTH)),
            static_cast<int>(cap.get(CAP_PROP_FRAME_HEIGHT)),
            blockSize,
            iFrameInterval,
            searchRange,
            cap.get(CAP_PROP_FPS),
            static_cast<int>(cap.get(CAP_PROP_FRAME_COUNT)),
            isLossy,
            quantizationLevel,
            targetBitrate
        };

        size_t originalSize = header.width * header.height * 3 * header.totalFrames;
        size_t compressedSize = 0;

        bitStream bs;
        bs.openFile(outputPath, ios::out | ios::binary);

        bs.writeBits(header.width, 16);
        bs.writeBits(header.height, 16);
        bs.writeBits(header.blockSize, 8);
        bs.writeBits(header.iFrameInterval, 8);
        bs.writeBits(header.searchRange, 8);
        bs.writeBits(static_cast<int>(header.fps), 16);
        bs.writeBits(header.totalFrames, 32);
        bs.writeBit(header.isLossy);
        if (header.isLossy) {
            bs.writeBits(header.quantizationLevel, 8);
            bs.writeBits(header.targetBitrate, 32);
        }

        Mat frame, prevFrame;
        int frameCount = 0;
        vector<Mat> originalFrames; // PSNR calculation

        while (cap.read(frame)) {
            if (frame.empty()) break;
            
            Mat originalFrame = frame.clone();
            originalFrames.push_back(originalFrame);

            bool isIFrame = (frameCount % iFrameInterval == 0);
            bs.writeBit(isIFrame);

            if (isIFrame) {
                if (isLossy) {
                    // Apply DCT and quantization for I-frames in lossy mode
                    for (int y = 0; y < frame.rows; y += 8) {
                        for (int x = 0; x < frame.cols; x += 8) {
                            Rect blockRect(x, y, min(8, frame.cols - x), 
                                         min(8, frame.rows - y));
                            Mat block = frame(blockRect).clone();
                            
                            vector<Mat> channels;
                            split(block, channels);
                            
                            for (int c = 0; c < 3; c++) {
                                Mat padded;
                                copyMakeBorder(channels[c], padded, 0, 8 - blockRect.height,
                                             0, 8 - blockRect.width, BORDER_CONSTANT, 0);
                                
                                dct2D(padded);
                                quantizeBlock(padded, c == 0);
                                
                                vector<int> zigZag = zigZagOrder(padded);
                                for (int coeff : zigZag) {
                                    bs.writeBits(coeff + 128, 8);
                                }
                            }
                        }
                    }
                } else {
                    vector<Mat> channels;
                    split(frame, channels);
                    for (int c = 0; c < 3; c++) {
                        for (int i = 0; i < frame.rows; i++) {
                            for (int j = 0; j < frame.cols; j++) {
                                bs.writeBits(channels[c].at<uchar>(i,j), 8);
                            }
                        }
                    }
                }
                frame.copyTo(prevFrame);
            } else {// P-frames
                for (int y = 0; y < frame.rows; y += blockSize) {
                    for (int x = 0; x < frame.cols; x += blockSize) {
                        int currBlockWidth = min(blockSize, frame.cols - x);
                        int currBlockHeight = min(blockSize, frame.rows - y);
                        Rect blockRect(x, y, currBlockWidth, currBlockHeight);
                        
                        Mat currentBlock = frame(blockRect);
                        Mat predictedBlock;
                        Point2i mv;

                        if (!isLossy) {
                            // Lossless mode - standard motion estimation
                            mv = estimateMotion(frame, prevFrame, x, y, blockSize, searchRange);
                            
                            int predX = max(0, min(frame.cols - currBlockWidth, x + mv.x));
                            int predY = max(0, min(frame.rows - currBlockHeight, y + mv.y));
                            predictedBlock = prevFrame(Rect(predX, predY, currBlockWidth, currBlockHeight));
                            
                            // Motion vector
                            bs.writeBits(mv.x + 128, 8);
                            bs.writeBits(mv.y + 128, 8);
                            
                            // Calculate and encode residuals
                            Mat residual = calculateResidual(currentBlock, predictedBlock);
                            vector<Mat> channels;
                            split(residual, channels);
                            
                            for (int c = 0; c < 3; c++) {
                                for (int i = 0; i < currBlockHeight; i++) {
                                    for (int j = 0; j < currBlockWidth; j++) {
                                        short value = channels[c].at<short>(i,j);
                                        value = static_cast<short>(max(-128, min(127, static_cast<int>(value))));
                                        bs.writeBits(value + 128, 8);
                                    }
                                }
                            }
                        } else {
                            // Lossy mode - DCT-based coding
                            mv = estimateMotion(frame, prevFrame, x, y, blockSize, searchRange);
                            
                            //Motion vector
                            bs.writeBits(mv.x + 128, 8);
                            bs.writeBits(mv.y + 128, 8);
                            
                            int predX = max(0, min(frame.cols - currBlockWidth, x + mv.x));
                            int predY = max(0, min(frame.rows - currBlockHeight, y + mv.y));
                            predictedBlock = prevFrame(Rect(predX, predY, currBlockWidth, currBlockHeight));
                            
                            Mat residual = calculateResidual(currentBlock, predictedBlock);
                            vector<Mat> channels;
                            split(residual, channels);
                            
                            for (int c = 0; c < 3; c++) {
                                Mat padded;
                                copyMakeBorder(channels[c], padded, 0, 8 - channels[c].rows % 8,
                                             0, 8 - channels[c].cols % 8, BORDER_CONSTANT, 0);
                                
                                for (int by = 0; by < padded.rows; by += 8) {
                                    for (int bx = 0; bx < padded.cols; bx += 8) {
                                        Mat block = padded(Rect(bx, by, 8, 8));
                                        dct2D(block);
                                        quantizeBlock(block, c == 0);
                                        
                                        vector<int> zigZag = zigZagOrder(block);
                                        for (int coeff : zigZag) {
                                            bs.writeBits(coeff + 128, 8);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                frame.copyTo(prevFrame);
            }
            frameCount++;
            
            // Rate control for lossy mode
            if (isLossy && targetBitrate > 0) {
                double currentBitrate = getTotalBits() / frameCount;
                adjustQuantizationLevel(currentBitrate);
            }
        }
        
        auto endTime = high_resolution_clock::now();
        metrics.encodingTime = duration_cast<milliseconds>(endTime - startTime).count() / 1000.0;
        
        if (!originalFrames.empty()) {
            metrics.calculatePSNR(originalFrames.back(), prevFrame);
        }
        
        size_t compressed_Size = getTotalBits() / 8;
        metrics.calculateCompressionRatio(originalSize, compressed_Size);
        
        cap.release();
        bs.flushBuffer();
        
        return metrics;
    }

    void decodeVideo(const string& inputPath, const string& outputPath) {
        bitStream bs;
        bs.openFile(inputPath, ios::in | ios::binary);

        VideoHeader header;
        header.width = bs.readBits(16);
        header.height = bs.readBits(16);
        header.blockSize = bs.readBits(8);
        header.iFrameInterval = bs.readBits(8);
        header.searchRange = bs.readBits(8);
        header.fps = bs.readBits(16);
        header.totalFrames = bs.readBits(32);
        header.isLossy = bs.readBit();
        
        if (header.isLossy) {
            header.quantizationLevel = bs.readBits(8);
            header.targetBitrate = bs.readBits(32);
        }

        VideoWriter writer(outputPath, VideoWriter::fourcc('a','v','c','1'), 
                         header.fps, Size(header.width, header.height));
        
        Mat frame(header.height, header.width, CV_8UC3);
        Mat prevFrame(header.height, header.width, CV_8UC3);

        for (int frameCount = 0; frameCount < header.totalFrames; frameCount++) {
            bool isIFrame = bs.readBit();

            if (isIFrame) {
                decodeIFrame(bs, frame, header);
            } else {
                decodePFrame(bs, frame, prevFrame, header);
            }
            
            writer.write(frame);
            frame.copyTo(prevFrame);
        }
        
        writer.release();
    }

};
// int main() {
//     try {
//         string inputPath = "../proj2/input/videos/bus_cif.y4m";
//         string encodedPath = "../proj2/output/encoded_bus.bin";
//         string outputPath = "../proj2/output/decoded_bus.mp4";
//         int iFrameInterval = 10;
//         int blockSize = 16;
//         int searchRange = 16;

//         VideoCoder coder;
//         cout << "Encoding video..." << endl;
//         EncodingMetrics metrics = coder.encodeVideo(inputPath, encodedPath, iFrameInterval, blockSize, searchRange);

//         cout << "Encoding completed." << endl;
//         cout << "PSNR: " << metrics.psnr << " dB" << endl;
//         cout << "MSE: " << metrics.mse << endl;
//         cout << "Compression Ratio: " << metrics.compressionRatio << endl;
//         cout << "Encoding Time: " << metrics.encodingTime << " seconds" << endl;

//         cout << "Decoding video..." << endl;
//         auto decodeStartTime = chrono::high_resolution_clock::now();
//         coder.decodeVideo(encodedPath, outputPath);
//         auto decodeEndTime = chrono::high_resolution_clock::now();
//         double decodeTime = chrono::duration_cast<chrono::milliseconds>(decodeEndTime - decodeStartTime).count() / 1000.0;

//         cout << "Decoding completed." << endl;       
//         cout << "Decoding Time: " << decodeTime << " seconds" << endl;
//     } catch (const exception& e) {
//         cerr << "Error: " << e.what() << endl;
//         return 1;
//     }

//     return 0;
// }