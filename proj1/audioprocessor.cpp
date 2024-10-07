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

using namespace std;

class AudioProcessor
{
private:
    sf::SoundBuffer buffer;
    sf::Sound sound;

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
        return true;
    }

    void playAudio()
    {
        sf::Sound sound;
        sound.setBuffer(buffer);
        sound.play();
        while (sound.getStatus() == sf::Sound::Playing)
        {
            // wait for sound to finish
        }
    }

    //get information about the audio file
    void getAudioInfo()
    {
        cout << "File info: " << endl;
        cout << "Duration: " << buffer.getDuration().asSeconds() << " seconds" << endl;
        cout << "Sample rate: " << buffer.getSampleRate() << endl;
        cout << "Channel count: " << buffer.getChannelCount() << endl;
    }

    void plotAudio()
    {
        //plot audio data
        const sf::Int16 *samples = buffer.getSamples();
        size_t count = buffer.getSampleCount();
        for (size_t i = 0; i < count; i++)
        {
            cout << samples[i] << " ";
        }
    }
};

int main(int argc, char *argv[])
{
    AudioProcessor p;
    if (argc != 2)
    {
        p.readFile("a.wav");
    }
    else
    {
        p.readFile(argv[1]);
    }
    p.playAudio();
    p.getAudioInfo();
    return 0;
}