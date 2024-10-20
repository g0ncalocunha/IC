#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <clocale>
#include <cwctype>
#include <codecvt>
#include <locale>
#include <SFML/Audio.hpp>
#include <fftw3.h>
#include <SFML/Graphics.hpp>
#include <thread>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <sstream>

using namespace std;

class AudioProcessor
{
private:
    sf::SoundBuffer buffer;
    sf::Sound sound;
    const sf::Int16 *samples;
    vector<sf::Int16> leftChannel;
    vector<sf::Int16> rightChannel;
    vector<sf::Int16> midChannel;
    vector<sf::Int16> sideChannel;
    vector<sf::Int16> targetChannel;
    vector<sf::Int16> quantizedSamples;
    unsigned int channelCount;
    const unsigned int WINDOW_WIDTH = 1920;
    const unsigned int WINDOW_HEIGHT = 800;
    const unsigned int LEFT_MARGIN = 100;
    const unsigned int RIGHT_MARGIN = 50;
    const unsigned int TOP_MARGIN = 50;
    const unsigned int BOTTOM_MARGIN = 100;

public:
    bool readFile(const string &file)
    {
        // open autf file
        ifstream infile(file);
        if (!infile.is_open())
        {
            cerr << "Error opening file" << endl;
            return false;
        }
        if (!buffer.loadFromFile(file))
        {
            cerr << "Error loading file" << endl;
            return false;
        }
        sound.setBuffer(buffer);
        return true;
    }

    void playAudio()
    {
        sound.play();
        while (sound.getStatus() == sf::Sound::Playing)
        {
            // wait for sound to finish
            this_thread::sleep_for(chrono::milliseconds(100));
        }
    }

    // get information about the audio file
    void getAudioInfo()
    {
        cout << "File info: " << endl;
        cout << "Duration: " << buffer.getDuration().asSeconds() << " seconds" << endl;
        cout << "Sample rate: " << buffer.getSampleRate() << endl;
        cout << "Channel count: " << buffer.getChannelCount() << endl;
    }

    void plotAudioWaveform()
    {
        const unsigned int GRAPH_HEIGHT = (WINDOW_HEIGHT - TOP_MARGIN - BOTTOM_MARGIN) / 2;

        sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Audio Waveform");
        sf::Font font;
        if (!font.loadFromFile("arial.ttf")) {
            cerr << "Error loading font!" << endl;
            return;
        }

        sf::Uint64 sampleCount = buffer.getSampleCount();
        unsigned int channelCount = buffer.getChannelCount();
        sf::Uint64 samplesPerChannel = sampleCount / channelCount;
        unsigned int sampleRate = buffer.getSampleRate();
        const sf::Int16* samples = buffer.getSamples();

        float duration = static_cast<float>(samplesPerChannel) / sampleRate;
        float scaleX = (WINDOW_WIDTH - LEFT_MARGIN - RIGHT_MARGIN) / duration;
        float scaleY = GRAPH_HEIGHT / 2.0f;  

        vector<sf::VertexArray> waveforms(channelCount, sf::VertexArray(sf::LinesStrip, samplesPerChannel));
        for (unsigned int channel = 0; channel < channelCount; ++channel) {
            float channelOffset = TOP_MARGIN + GRAPH_HEIGHT * (channel + 0.5f);
            for (sf::Uint64 i = 0; i < samplesPerChannel; ++i) {
                float timeInSeconds = static_cast<float>(i) / sampleRate;
                float x = LEFT_MARGIN + timeInSeconds * scaleX;
                float sampleValue = static_cast<float>(samples[i * channelCount + channel]) / 32768.0f;
                float y = channelOffset - sampleValue * scaleY;
                waveforms[channel][i].position = sf::Vector2f(x, y);
                waveforms[channel][i].color = sf::Color(50 * (channel + 1), 50 * (channel + 2), 250);;  
            }
        }

        vector<sf::VertexArray> axes;
        for (unsigned int channel = 0; channel < channelCount; ++channel) {
            float yPosition = TOP_MARGIN + GRAPH_HEIGHT * (channel + 1);
            
            sf::VertexArray xAxis(sf::Lines, 2);
            xAxis[0].position = sf::Vector2f(LEFT_MARGIN, yPosition);
            xAxis[1].position = sf::Vector2f(WINDOW_WIDTH - RIGHT_MARGIN, yPosition);
            xAxis[0].color = xAxis[1].color = sf::Color::Black;
            axes.push_back(xAxis);

            sf::VertexArray yAxis(sf::Lines, 2);
            yAxis[0].position = sf::Vector2f(LEFT_MARGIN, yPosition - GRAPH_HEIGHT);
            yAxis[1].position = sf::Vector2f(LEFT_MARGIN, yPosition);
            yAxis[0].color = yAxis[1].color = sf::Color::Black;
            axes.push_back(yAxis);
        }

        sf::Text xAxisLabel("Time (s)", font, 20);
        xAxisLabel.setFillColor(sf::Color::Black);
        xAxisLabel.setPosition(WINDOW_WIDTH / 2.0f -40, WINDOW_HEIGHT - (BOTTOM_MARGIN / 2.0f) + 10);

        sf::Text yAxisLabel("Amplitude", font, 20);
        yAxisLabel.setFillColor(sf::Color::Black);
        yAxisLabel.setPosition(LEFT_MARGIN-80, WINDOW_HEIGHT/2.0f);
        yAxisLabel.setRotation(-90);

        vector<sf::Text> timeTicks;
        vector<sf::VertexArray> timeLines;
        int majorTickInterval = 1; 
        int minorTicksPerMajor = 4; 

        for (int i = 0; i <= static_cast<int>(duration); ++i) {
            sf::Text tickLabel(to_string(i), font, 18);
            tickLabel.setFillColor(sf::Color::Black);
            tickLabel.setPosition(LEFT_MARGIN + i * scaleX, (WINDOW_HEIGHT - BOTTOM_MARGIN / 2.0f ) -20 );
            timeTicks.push_back(tickLabel);

            sf::VertexArray majorLine(sf::Lines, 2);
            majorLine[0].position = sf::Vector2f(LEFT_MARGIN + i * scaleX, WINDOW_HEIGHT - BOTTOM_MARGIN);
            majorLine[1].position = sf::Vector2f(LEFT_MARGIN + i * scaleX, WINDOW_HEIGHT - BOTTOM_MARGIN + 10);
            majorLine[0].color = majorLine[1].color = sf::Color::Black;
            timeLines.push_back(majorLine);

            if (i < static_cast<int>(duration)) {
                for (int j = 1; j < minorTicksPerMajor; ++j) {
                    float minorX = LEFT_MARGIN + (i + static_cast<float>(j) / minorTicksPerMajor) * scaleX;
                    sf::VertexArray minorLine(sf::Lines, 2);
                    minorLine[0].position = sf::Vector2f(minorX, WINDOW_HEIGHT - BOTTOM_MARGIN);
                    minorLine[1].position = sf::Vector2f(minorX, WINDOW_HEIGHT - BOTTOM_MARGIN + 5);
                    minorLine[0].color = minorLine[1].color = sf::Color(200, 200, 200);
                    timeLines.push_back(minorLine);
                }
            }
        }

        vector<vector<sf::Text>> amplitudeTicks(channelCount);
        int tickCountY = 5;
        float tickPadding = 15.0f;
        for (unsigned int channel = 0; channel < channelCount; ++channel) {
            for (int i = 0; i <= tickCountY; ++i) {
                float amplitude = 1.0f - (2.0f * i / tickCountY);
                ostringstream amplitudeStream;
                amplitudeStream << fixed << setprecision(2) << amplitude;
                sf::Text tick(amplitudeStream.str(), font, 15);
                tick.setFillColor(sf::Color::Black);

                float yPos = TOP_MARGIN + GRAPH_HEIGHT * channel + i * (GRAPH_HEIGHT / tickCountY);
                if (i==0){
                    yPos+=tickPadding;
                }else if(i==tickCountY){
                    yPos-=tickPadding;
                }
                tick.setPosition(LEFT_MARGIN - 40, yPos);
                amplitudeTicks[channel].push_back(tick);
            }
        }

        vector<sf::Text> amplitudeLabels(channelCount);
        for (unsigned int channel = 0; channel < channelCount; ++channel) {
            sf::Text label("Amplitude", font, 18);
            label.setFillColor(sf::Color::Black);
            float yPos = TOP_MARGIN + GRAPH_HEIGHT * (channel + 0.5f);
            label.setPosition(LEFT_MARGIN - 80, yPos);
            label.setRotation(-90);
            amplitudeLabels[channel] = label;
        }

        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) window.close();
            }

            window.clear(sf::Color::White);
            for (const auto& axis : axes) window.draw(axis);
            for (const auto& waveform : waveforms) window.draw(waveform);
            for (const auto& line : timeLines) window.draw(line);
            for (const auto& tick : timeTicks) window.draw(tick);
            for (const auto& channelTicks : amplitudeTicks) {
                for (const auto& tick : channelTicks) window.draw(tick);
            }
            window.draw(xAxisLabel);
            window.draw(yAxisLabel);
            window.display();
        }
    }

    void getLeftRightChannels()
    {
        samples=buffer.getSamples();
        sf::Uint64 sampleCount = buffer.getSampleCount();
        if (channelCount == 2)
        {
            leftChannel.reserve(sampleCount / 2);
            rightChannel.reserve(sampleCount / 2);
            for (sf::Uint64 i = 0; i < sampleCount; i += 2)
            {
                leftChannel.push_back(samples[i]);
                rightChannel.push_back(samples[i + 1]);
            }
        }
    }

    void getMIDchannel()
    {
        for (size_t i = 0; i < leftChannel.size(); ++i)
        {
            int mid = (leftChannel[i] + rightChannel[i]) / 2;
            int side = (leftChannel[i] - rightChannel[i]) / 2;
            midChannel.push_back(mid);
            sideChannel.push_back(side);
        }
    }

    void plotHistogram(bool quantized, string title)
    {
        channelCount = buffer.getChannelCount();
        if (quantized)
        {
            targetChannel = quantizedSamples;
        }

        if (channelCount < 2)
        {
            cerr << "Error: Channel count is not stereo." << endl;
            return;
        }

        if (title == "Right Channel")
        {
            getLeftRightChannels();
            targetChannel = rightChannel;
        }
        else if (title == "Left Channel")
        {
            getLeftRightChannels();
            targetChannel = leftChannel;
        }
        else if (title == "Mid Channel")
        {
            getMIDchannel();
            targetChannel = midChannel;
        }
        else
        {
            getMIDchannel();
            targetChannel = sideChannel; 
        }

        if (targetChannel.empty())
        {
            cerr << "Error: targetChannel is empty." << endl;
            return;
        }

        vector<int> histogram(256, 0);  // Ajusta o histograma para 256 bins
        int binSize = 256;  // 256 bins para lidar com valores de 16 bits

        sf::Color color = quantized ? sf::Color::Blue : sf::Color::Magenta;

        int windowWidth = 950;
        int windowHeight = 650;
        int binWidth = windowWidth / histogram.size();
        int maxHeight = 400;

        sf::Font font;
        if (!font.loadFromFile("arial.ttf"))
        {
            cerr << "Error loading font." << endl;
        }

        sf::RenderWindow window(sf::VideoMode(windowWidth, windowHeight), title + " Histogram");

        for (auto sample : targetChannel)
        {
            int binIndex = (sample + 32768) / binSize;
            if (binIndex >= 0 && binIndex < static_cast<int>(histogram.size()))
            {
                histogram[binIndex]++;
            }
            else
            {
                cerr << "ERROR: Out of bounds access in histogram" << endl;
            }
        }

        int maxBinValue = *max_element(histogram.begin(), histogram.end());

        int totalBinsWidth = histogram.size() * binWidth;
        int startX = (windowWidth - totalBinsWidth) / 2;  // Centrando histograma
        int startY = windowHeight - 50; 

        sf::Text xLabel("Amplitude", font, 14);
        xLabel.setFillColor(sf::Color::Black);
        xLabel.setPosition(windowWidth / 2, windowHeight - 35);  

        sf::Text yLabel("Frequency", font, 14);
        yLabel.setFillColor(sf::Color::Black);
        yLabel.setPosition(20, windowHeight / 2 - 20);  
        yLabel.setRotation(-90);

        sf::Vertex xAxis[] =
        {
            sf::Vertex(sf::Vector2f(startX, startY - 20)),
            sf::Vertex(sf::Vector2f(startX + totalBinsWidth, startY - 20))  
        };

        sf::Vertex yAxis[] =
        {
            sf::Vertex(sf::Vector2f(startX, startY)),
            sf::Vertex(sf::Vector2f(startX, startY - windowHeight))  
        };

        xAxis[0].color = xAxis[1].color = sf::Color::Black;
        yAxis[0].color = yAxis[1].color = sf::Color::Black;

        vector<sf::Text> yTickLabels;
        vector<sf::Vertex> yTicks;
        int numYTicks = 5;
        for (int i = 0; i <= numYTicks; ++i)
        {
            float yPos = startY - (i * maxHeight / numYTicks) - 20;  

            yTicks.push_back(sf::Vertex(sf::Vector2f(startX - 5, yPos), sf::Color::Black));
            yTicks.push_back(sf::Vertex(sf::Vector2f(startX + 5, yPos), sf::Color::Black));

            sf::Text label(to_string(i * maxBinValue / numYTicks), font, 12);
            label.setFillColor(sf::Color::Black);
            label.setPosition(startX - 40, yPos - 10);
            yTickLabels.push_back(label);
        }

        vector<sf::Text> xTickLabels;
        vector<sf::Vertex> xTicks;
        int numXTicks = 10;
        float xTickInterval = static_cast<float>(histogram.size()) / numXTicks;

        for (int i = 0; i <= numXTicks; ++i)
        {
            float xPos = startX + i * (totalBinsWidth / numXTicks);
            
            xTicks.push_back(sf::Vertex(sf::Vector2f(xPos, startY - 25), sf::Color::Black));  // Subir ticks X (-20px)
            xTicks.push_back(sf::Vertex(sf::Vector2f(xPos, startY - 15), sf::Color::Black));  // Subir ticks X (-20px)

            int labelValue = static_cast<int>(i * (256 / numXTicks));
            sf::Text label(to_string(labelValue), font, 12);
            label.setFillColor(sf::Color::Black);
            label.setPosition(xPos - 10, startY + 10 - 20); 
            xTickLabels.push_back(label);
        }

        while (window.isOpen())
        {
            sf::Event event;
            while (window.pollEvent(event))
            {
                if (event.type == sf::Event::Closed)
                {
                    window.close();
                }
            }

            window.clear(sf::Color::White);

            // Desenhar eixos
            window.draw(xAxis, 2, sf::Lines);
            window.draw(yAxis, 2, sf::Lines);

            // Desenhar ticks do eixo Y e seus rótulos
            for (size_t i = 0; i < yTicks.size(); i += 2)
            {
                window.draw(&yTicks[i], 2, sf::Lines);
            }
            for (const auto& label : yTickLabels)
            {
                window.draw(label);
            }

            // Desenhar ticks do eixo X e seus rótulos
            for (size_t i = 0; i < xTicks.size(); i += 2)
            {
                window.draw(&xTicks[i], 2, sf::Lines);
            }
            for (const auto& label : xTickLabels)
            {
                window.draw(label);
            }

            // Desenhar as barras 
            for (size_t i = 0; i < histogram.size(); ++i)
            {
                float binHeight = static_cast<float>(histogram[i]) / maxBinValue * maxHeight;
                sf::RectangleShape bar(sf::Vector2f(binWidth - 1, -binHeight));
                bar.setPosition(startX + i * binWidth, startY -20);
                bar.setFillColor(color);
                window.draw(bar);
            }

            // Desenhar rótulos dos eixos
            window.draw(xLabel);
            window.draw(yLabel);

            window.display();
        }
    }

    void quantization(int n)
    {
        int sampleCount = buffer.getSampleCount();
        int max_value = *max_element(samples, samples + sampleCount);
        int min_value = *min_element(samples, samples + sampleCount);

        int numLevels = 1 << n;
        float step_size = static_cast<float>(max_value - min_value) / (numLevels - 1);

        quantizedSamples.resize(sampleCount);

        for (size_t i = 0; i < sampleCount; ++i)
        {
            float normalizedSample = (samples[i] - min_value) / step_size;
            quantizedSamples[i] = static_cast<sf::Int16>(round(normalizedSample)) * step_size + min_value;
        }
    }

    void compareAudios()
    {
        //compare quantized audio with original audio
        sf::Uint64 sampleCount = buffer.getSampleCount();
        
        //Mean Squared Error
        double mse = 0;
        for (sf::Uint64 i = 0; i < sampleCount; ++i)
        {
            mse += pow(samples[i] - quantizedSamples[i], 2);
        }
        mse /= sampleCount;
        cout << "Mean Squared Error: " << mse << endl;
        //Signal to Noise Ratio
        double snr = 0;
        for (sf::Uint64 i = 0; i < sampleCount; ++i)
        {
            snr += pow(samples[i], 2);
        }
        snr /= mse;
        cout << "Signal to Noise Ratio: " << snr << endl;

    }

    void frequencyAnalyser()
    {
        // Fourier Transform
        samples = buffer.getSamples();
        sf::Uint64 sampleCount = buffer.getSampleCount();
        unsigned int channelCount = buffer.getChannelCount();
        sf::Uint64 samplesPerChannel = sampleCount / channelCount;

        // Use FFTW library for Fourier Transform
        vector<double> real(samplesPerChannel);
        vector<double> imag(samplesPerChannel, 0.0);

        for (sf::Uint64 i = 0; i < samplesPerChannel; ++i)
        {
            real[i] = samples[i * channelCount] / 32768.0; // Normalize sample
        }

        // Perform FFT
        fftw_complex *in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * samplesPerChannel);
        fftw_complex *out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * samplesPerChannel);
        fftw_plan p = fftw_plan_dft_1d(samplesPerChannel, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

        for (sf::Uint64 i = 0; i < samplesPerChannel; ++i)
        {
            in[i][0] = real[i];
            in[i][1] = imag[i];
        }

        fftw_execute(p);

        // Calculate magnitude
        vector<double> magnitude(samplesPerChannel);
        for (sf::Uint64 i = 0; i < samplesPerChannel; ++i)
        {
            magnitude[i] = sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1]);
        }

        fftw_destroy_plan(p);
        fftw_free(in);
        fftw_free(out);

        // Plot the frequency spectrum
        sf::RenderWindow window(sf::VideoMode(1920, 600), "Frequency Spectrum");

        sf::VertexArray spectrum(sf::LinesStrip, samplesPerChannel);
        float scaleX = static_cast<float>(window.getSize().x) / samplesPerChannel;
        float scaleY = window.getSize().y / *max_element(magnitude.begin(), magnitude.end());

        for (sf::Uint64 i = 0; i < samplesPerChannel; ++i)
        {
            float x = i * scaleX;
            float y = magnitude[i] * scaleY;
            spectrum[i].position = sf::Vector2f(x, window.getSize().y - y);
            spectrum[i].color = sf::Color::Green;
        }

        while (window.isOpen())
        {
            sf::Event event;
            while (window.pollEvent(event))
            {
                if (event.type == sf::Event::Closed)
                {
                    window.close();
                }
            }
            window.clear(sf::Color::Black);
            window.draw(spectrum);
            window.display();
        }
    }

    void noiseAdder(const string &filename){
        //add noise to audio
        sf::Uint64 sampleCount = buffer.getSampleCount();
        unsigned int channelCount = buffer.getChannelCount();
        sf::Uint64 samplesPerChannel = sampleCount / channelCount;

        // Add white noise
        vector<sf::Int16> noisySamples(sampleCount);
        for (sf::Uint64 i = 0; i < sampleCount; ++i)
        {
            noisySamples[i] = samples[i] + (rand() % 32768 - 16384); // Add random noise
        }

        // Save noisy audio to a new file
        sf::SoundBuffer noisyBuffer;
        noisyBuffer.loadFromSamples(&noisySamples[0], sampleCount, channelCount, buffer.getSampleRate());
        //save to directory audioprocessor_files
        noisyBuffer.saveToFile(filename.substr(0, filename.find_last_of('.')) + "Noisy.wav");
        cout << "Noisy audio saved to " << filename.substr(0, filename.find_last_of('.')) + "Noisy.wav" << endl;
        // Load noisy audio from file
        sf::SoundBuffer noisyBuffer2;

        //MSE and SNR
        double mse = 0;
        for (sf::Uint64 i = 0; i < sampleCount; ++i)
        {
            mse += pow(samples[i] - noisySamples[i], 2);
        }
        mse /= sampleCount;
        cout << "Mean Squared Error: " << mse << endl;
        //Signal to Noise Ratio
        double snr = 0;
        for (sf::Uint64 i = 0; i < sampleCount; ++i)
        {
            snr += pow(samples[i], 2);
        }
        snr /= mse;
        cout << "Signal to Noise Ratio: " << snr << endl;
        
    }
};

int main(int argc, char *argv[])
{
    AudioProcessor p;
    string filename;
    string filename2;

    if (argc != 2)
    {
        filename = "audioprocessor_files/sample04.wav";
    }
    else
    {
        filename = argv[1];
    }

    if (!p.readFile(filename))
    {
        cerr << "Failed to load audio file." << endl;
        return -1;
    }

    p.playAudio();
    p.getAudioInfo();
    p.plotAudioWaveform();
    p.plotHistogram(false, "Right Channel");
    p.plotHistogram(false, "Left Channel");
    p.plotHistogram(false, "Mid Channel");
    p.plotHistogram(false, "Side Channel");
    p.quantization(8);
    p.plotHistogram(true, "Quantization");
    p.compareAudios();
    p.frequencyAnalyser();
    p.noiseAdder(filename);

    return 0;
}