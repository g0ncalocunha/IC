#include <sndfile.h>
#include <vector>
#include <iostream>
#include "bitStream.cpp"
#include "golomb.cpp"

using namespace std;

class AudioCodec {
public:
    void encode(const string &inputFile, const string &outputFile, uint32_t m, bool adaptive = false);
    void decode(const string &inputFile, const string &outputFile);

private:
    void predictMono(const vector<int16_t> &samples, vector<int16_t> &residuals);
    void predictStereo(const vector<int16_t> &left, const vector<int16_t> &right, vector<int16_t> &leftResiduals, vector<int16_t> &rightResiduals);
    void calculateOptimalM(const vector<int16_t> &residuals, uint32_t &m);
};

void AudioCodec::encode(const string &inputFile, const string &outputFile, uint32_t m, bool adaptive) {
    SF_INFO sfInfo;
    SNDFILE *inFile = sf_open(inputFile.c_str(), SFM_READ, &sfInfo);
    if (!inFile) {
        cerr << "Failed to open input file: " << inputFile << endl;
        return;
    }

    vector<int16_t> samples(sfInfo.frames * sfInfo.channels);
    sf_read_short(inFile, samples.data(), samples.size());
    sf_close(inFile);

    bitStream bs;
    bs.openFile(outputFile);

    vector<int16_t> residuals;
    if (sfInfo.channels == 1) {
        predictMono(samples, residuals);
    } else if (sfInfo.channels == 2) {
        vector<int16_t> left(samples.size() / 2), right(samples.size() / 2);
        for (size_t i = 0; i < samples.size(); i += 2) {
            left[i / 2] = samples[i];
            right[i / 2] = samples[i + 1];
        }
        vector<int16_t> leftResiduals, rightResiduals;
        predictStereo(left, right, leftResiduals, rightResiduals);
        residuals.insert(residuals.end(), leftResiduals.begin(), leftResiduals.end());
        residuals.insert(residuals.end(), rightResiduals.begin(), rightResiduals.end());
    }

    if (adaptive) {
        calculateOptimalM(residuals, m);
    }

    golomb g;
    g.setM(m);
    int pos = 0;
    for (int16_t residual : residuals) {
        g.encodeGolomb(residual, bs, pos);
    }

    bs.flushBuffer();
}

void AudioCodec::decode(const string &inputFile, const string &outputFile) {
    // Implementar a decodificação
}

void AudioCodec::predictMono(const vector<int16_t> &samples, vector<int16_t> &residuals) {
    residuals.resize(samples.size());
    residuals[0] = samples[0];
    for (size_t i = 1; i < samples.size(); ++i) {
        residuals[i] = samples[i] - samples[i - 1];
    }
}

void AudioCodec::predictStereo(const vector<int16_t> &left, const vector<int16_t> &right, vector<int16_t> &leftResiduals, vector<int16_t> &rightResiduals) {
    leftResiduals.resize(left.size());
    rightResiduals.resize(right.size());
    leftResiduals[0] = left[0];
    rightResiduals[0] = right[0];
    for (size_t i = 1; i < left.size(); ++i) {
        leftResiduals[i] = left[i] - left[i - 1];
        rightResiduals[i] = right[i] - right[i - 1];
    }
}

void AudioCodec::calculateOptimalM(const vector<int16_t> &residuals, uint32_t &m) {
    // to do
}

int main() {
    AudioCodec codec;
    codec.encode("input.wav", "encoded.bin", 10, true);
    codec.decode("encoded.bin", "output.wav");
    return 0;
}