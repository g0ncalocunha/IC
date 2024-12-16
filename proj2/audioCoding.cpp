#include <iostream>
#include <vector>
#include <cstdint>
#include <cmath>
#include <fstream>
#include <string>
#include <algorithm>
#include <filesystem>
#include "golomb.cpp"
#include "audioReader.cpp"
class audioCodec {
private:
    int m; // Parâmetro de Golomb
    bool adaptive; // Se o parâmetro m é adaptativo
    golomb golombEncoder; // Codificador Golomb
    golomb golombDecoder; // Decodificador Golomb
    audioReader p;
    bitStream bs;

    void updateM(const std::vector<int>& residuals) {
        // Implementar lógica para atualizar m dinamicamente
        // Exemplo simples: média dos valores absolutos dos resíduos
        int sum = 0;
        for (int r : residuals) {
            sum += std::abs(r);
        }
        int newM = std::max(1, static_cast<int>(sum / residuals.size()));
        golombEncoder.setM(newM);
        golombDecoder.setM(newM);
    }

public:
    audioCodec(int m, bool adaptive = false) : m(m), adaptive(adaptive), golombEncoder(m), golombDecoder(m) {}

    void encode(const std::string& inputFile, const std::string& outputFile) {
        p.readFile(inputFile);

        bs.openFile(outputFile);

        const sf::Int16* samples = p.getSamples(inputFile);
        int sampleCount = p.getSampleCount(inputFile);
        int channelCount = p.getChannelCount(inputFile);

        std::vector<int> residuals;

        for (int i = 0; i < sampleCount; ++i) {
            int prediction = 0;
            if (i > 0) {
                prediction = samples[i - 1];
            }
            int residual = samples[i] - prediction;
            residuals.push_back(residual);
            golombEncoder.encode(residual, bs);
        }

        if (adaptive) {
            updateM(residuals);
        }

        bs.flushBuffer();
    }

    void decode(const std::string& inputFile, const std::string& outputFile) {
        bs.openFile(inputFile);

        std::vector<sf::Int16> samples;
        int sampleCount = p.getSampleCount(inputFile);
        int channelCount = p.getChannelCount(inputFile);

        for (int i = 0; i < sampleCount; ++i) {
            int residual = golombDecoder.decode(bs);
            int prediction = 0;
            if (i > 0) {
                prediction = samples[i - 1];
            }
            int sample = residual + prediction;
            samples.push_back(sample);
        }

        // Salvar samples no arquivo de saída (implementar conforme necessário)
    }
};

int main(int argc, char const *argv[])
{
    
    audioCodec codec(10, true);
    codec.encode("sample01.wav", "encoded.bin");
    codec.decode("encoded.bin", "decoded.wav");
    return 0;
}
