#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <fstream>
#include <thread>
#include "imageCoder.h"
#include "bitStream.h"

using namespace std;
using namespace cv;

struct VideoHeader {
    int width;
    int height;
    int blockSize;
    int iFrameInterval;
    int searchRange;
    double fps;
    int totalFrames;
};

class VideoCoder {
private:
    ImageCoder imageCoder;  
    
    int mapValue(int value) {
        value = max(-255, min(255, value));
        return value + 255;  
    }

     Mat calculateResidual(const Mat& current, const Mat& predicted) {
        Mat residual;
        subtract(current, predicted, residual, noArray(), CV_16SC3);
        return residual;
    }

    Mat reconstructBlock(const Mat& predicted, const Mat& residual) {
        Mat result;
        add(predicted, residual, result, noArray(), CV_8UC3);
        // Range válido
        result.convertTo(result, CV_8UC3, 1.0, 0.0);
        return result;
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

    vector<vector<int>> getResidualVector(const Mat& currentBlock, const Mat& predictedBlock) {
        Mat residual;
        absdiff(currentBlock, predictedBlock, residual);
        
        vector<vector<int>> result(residual.channels());
        for(int c = 0; c < residual.channels(); c++) {
            result[c].reserve(residual.rows * residual.cols + 2);
            result[c].push_back(residual.rows);
            result[c].push_back(residual.cols);
            
            for(int i = 0; i < residual.rows; i++) {
                for(int j = 0; j < residual.cols; j++) {
                    int val = currentBlock.at<Vec3b>(i, j)[c] - 
                             predictedBlock.at<Vec3b>(i, j)[c];
                    result[c].push_back(mapValue(val));
                }
            }
        }
        return result;
    }
    
    vector<vector<int>> matToVector(const Mat& frame) {
        vector<vector<int>> result(frame.channels());
        for (int c = 0; c < frame.channels(); c++) {
            result[c].reserve(frame.rows * frame.cols + 2);
            result[c].push_back(frame.rows);
            result[c].push_back(frame.cols);

            for (int i = 0; i < frame.rows; i++) {
                for (int j = 0; j < frame.cols; j++) {
                    int val = frame.at<Vec3b>(i, j)[c];
                    result[c].push_back(val);
                }
            }
        }
        return result;
    }

    Mat vectorToMat(const vector<vector<int>>& vec) {
        if (vec.empty() || vec[0].size() < 2) {
            throw runtime_error("Invalid vector format");
        }

        int rows = vec[0][0];
        int cols = vec[0][1];
        Mat result(rows, cols, CV_8UC3);

        int idx = 2; 
        for (int c = 0; c < 3; c++) {  //3 canais (RGB ou YUV)
            for (int i = 0; i < rows; i++) {
                for (int j = 0; j < cols; j++) {
                    result.at<Vec3b>(i, j)[c] = vec[c][idx++];
                }
            }
        }
        return result;
    }


    void writeFrame(ofstream& outFile, const vector<vector<int>>& frameData) {
        for (const auto& channel : frameData) {
            size_t size = channel.size();
            outFile.write(reinterpret_cast<const char*>(&size), sizeof(size_t));
            outFile.write(reinterpret_cast<const char*>(channel.data()), size * sizeof(int));
        }
    }

    vector<vector<int>> readFrame(ifstream& inFile) {
        vector<vector<int>> frameData(3);
        for (auto& channel : frameData) {
            size_t size;
            inFile.read(reinterpret_cast<char*>(&size), sizeof(size_t));
            channel.resize(size);
            inFile.read(reinterpret_cast<char*>(channel.data()), size * sizeof(int));
        }
        return frameData;
    }

private:
    int blockSize;

    void encodeIntraBlock(const Mat& block, bitStream& bs) {
        auto residuals = imageCoder.calculateResiduals(block, Predictors::JPEG_LS);
        for (const auto& channel : residuals) {
            for (size_t i = 2; i < channel.size(); i++) {
                bs.writeBits(channel[i], 8);
            }
        }
    }

    void encodeInterBlock(const Mat& residual, const Point2i& motionVector, bitStream& bs) {
        bs.writeBits(motionVector.x + 32768, 16);  
        bs.writeBits(motionVector.y + 32768, 16);
        
        for (int i = 0; i < residual.rows; i++) {
            for (int j = 0; j < residual.cols; j++) {
                for (int c = 0; c < residual.channels(); c++) {
                    bs.writeBits(residual.at<Vec3b>(i, j)[c], 8);
                }
            }
        }
    }

    int calculateIntraBitrate(const Mat& block) {
        auto residuals = imageCoder.calculateResiduals(block, Predictors::JPEG_LS);
        return residuals[0].size() * 8;  // Total de bits = número de pixels * 8 bits/pixel
    }

    int calculateInterBitrate(const Mat& residual, const Point2i& motionVector) {
        return (residual.total() * residual.channels() * 8) + (2 * 16);  // Residual + motion vector
    }


public:
    VideoCoder(int bs = 16) : blockSize(bs), imageCoder() {}

    void encodeVideo(const string& inputPath, const string& outputPath, 
                    int iFrameInterval, int blockSize, int searchRange) {
        VideoCapture cap(inputPath);
        if (!cap.isOpened()) throw runtime_error("Cannot open input video");

        VideoHeader header = {
            static_cast<int>(cap.get(CAP_PROP_FRAME_WIDTH)),
            static_cast<int>(cap.get(CAP_PROP_FRAME_HEIGHT)),
            blockSize,
            iFrameInterval,
            searchRange,
            cap.get(CAP_PROP_FPS),
            static_cast<int>(cap.get(CAP_PROP_FRAME_COUNT))
        };

        bitStream bs;
        bs.openFile(outputPath, ios::out | ios::binary);

        // Header
        bs.writeBits(header.width, 16);
        bs.writeBits(header.height, 16);
        bs.writeBits(header.blockSize, 8);
        bs.writeBits(header.iFrameInterval, 8);
        bs.writeBits(header.searchRange, 8);
        bs.writeBits(static_cast<int>(header.fps), 16);
        bs.writeBits(header.totalFrames, 32);

        Mat frame, prevFrame;
        int frameCount = 0;

        while (cap.read(frame)) {
            if (frame.empty()) break;

            bool isIFrame = (frameCount % iFrameInterval == 0);
            bs.writeBit(isIFrame);

            if (isIFrame) {
                vector<Mat> channels;
                split(frame, channels);
                for (int c = 0; c < 3; c++) {
                    for (int i = 0; i < frame.rows; i++) {
                        for (int j = 0; j < frame.cols; j++) {
                            bs.writeBits(channels[c].at<uchar>(i,j), 8);
                        }
                    }
                }
                frame.copyTo(prevFrame);
            } else {
                for (int y = 0; y < frame.rows; y += blockSize) {
                    for (int x = 0; x < frame.cols; x += blockSize) {
                        int currBlockWidth = min(blockSize, frame.cols - x);
                        int currBlockHeight = min(blockSize, frame.rows - y);
                        Rect blockRect(x, y, currBlockWidth, currBlockHeight);
                        
                        Mat currentBlock = frame(blockRect);
                        Point2i mv = estimateMotion(frame, prevFrame, x, y, blockSize, searchRange);
                        
                        int predX = max(0, min(frame.cols - currBlockWidth, x + mv.x));
                        int predY = max(0, min(frame.rows - currBlockHeight, y + mv.y));
                        
                        bs.writeBits(mv.x + 128, 8);
                        bs.writeBits(mv.y + 128, 8);
                        
                        Mat predictedBlock = prevFrame(Rect(predX, predY, currBlockWidth, currBlockHeight));
                        Mat residual = calculateResidual(currentBlock, predictedBlock);
                        
                        vector<Mat> channels;
                        split(residual, channels);
                        for (int c = 0; c < 3; c++) {
                            for (int i = 0; i < currBlockHeight; i++) {
                                for (int j = 0; j < currBlockWidth; j++) {
                                    short value = channels[c].at<short>(i,j);
                                    // Garantir que o valor está no range -128 a 127
                                    value = static_cast<short>(std::max(-128, std::min(127, static_cast<int>(value))));
                                    bs.writeBits(value + 128, 8);
                                }
                            }
                        }
                    }
                }
                frame.copyTo(prevFrame);
            }
            frameCount++;
        }
        cap.release();
    }

    void decodeVideo(const string& inputPath, const string& outputPath) {
        bitStream bs;
        bs.openFile(inputPath, ios::in | ios::binary);

        int width = bs.readBits(16);
        int height = bs.readBits(16);
        int blockSize = bs.readBits(8);
        int iFrameInterval = bs.readBits(8);
        int searchRange = bs.readBits(8);
        int fps = bs.readBits(16);
        int totalFrames = bs.readBits(32);

        VideoWriter writer(outputPath, VideoWriter::fourcc('a','v','c','1'), fps, Size(width, height));
        
        Mat frame(height, width, CV_8UC3);
        Mat prevFrame(height, width, CV_8UC3);
        int frameCount = 0;

        while (frameCount < totalFrames) {
            bool isIFrame = bs.readBit();

            if (isIFrame) {
                vector<Mat> channels(3);
                for (int c = 0; c < 3; c++) {
                    channels[c] = Mat(height, width, CV_8UC1);
                    for (int i = 0; i < height; i++) {
                        for (int j = 0; j < width; j++) {
                            channels[c].at<uchar>(i,j) = bs.readBits(8);
                        }
                    }
                }
                merge(channels, frame);
                frame.copyTo(prevFrame);
            } else {
                for (int y = 0; y < height; y += blockSize) {
                    for (int x = 0; x < width; x += blockSize) {
                        int currBlockWidth = min(blockSize, width - x);
                        int currBlockHeight = min(blockSize, height - y);
                        
                        int mvX = bs.readBits(8) - 128;
                        int mvY = bs.readBits(8) - 128;
                        
                        int predX = max(0, min(width - currBlockWidth, x + mvX));
                        int predY = max(0, min(height - currBlockHeight, y + mvY));
                        
                        Mat predictedBlock = prevFrame(Rect(predX, predY, currBlockWidth, currBlockHeight));
                        Mat residual(currBlockHeight, currBlockWidth, CV_16SC3);
                        
                        vector<Mat> channels(3);
                        for (int c = 0; c < 3; c++) {
                            channels[c] = Mat(currBlockHeight, currBlockWidth, CV_16SC1);
                            for (int i = 0; i < currBlockHeight; i++) {
                                for (int j = 0; j < currBlockWidth; j++) {
                                    int value = bs.readBits(8);
                                    channels[c].at<short>(i,j) = value - 128;
                                }
                            }
                        }
                        merge(channels, residual);
                        
                        Mat reconstructedBlock = reconstructBlock(predictedBlock, residual);
                        reconstructedBlock.copyTo(frame(Rect(x, y, currBlockWidth, currBlockHeight)));
                    }
                }
                frame.copyTo(prevFrame);
            }
            
            writer.write(frame);
            frameCount++;
        }
        
        writer.release();
    }

};

int main() {
    try {
        string inputPath = "videos/akiyo_cif.y4m";
        string encodedPath = "encoded.bin";
        string outputPath = "output.mp4";
        int iFrameInterval = 10;
        int blockSize = 16;
        int searchRange = 16;

        VideoCoder coder;

        cout << "Encoding video..." << endl;
        coder.encodeVideo(inputPath, encodedPath, iFrameInterval, blockSize, searchRange);

        cout << "Decoding video..." << endl;
        coder.decodeVideo(encodedPath, outputPath);

    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
