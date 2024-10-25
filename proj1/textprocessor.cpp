#include <string>
#include <map>
#include <fstream>
#include <vector>
#include <iostream>
#include <algorithm>
#include <clocale>
#include <cwctype>
#include <matplotlibcpp.h>
#include <codecvt>
#include <locale>
#include <sstream>
#include <chrono>

namespace plt=matplotlibcpp;

using namespace std;
using namespace chrono;

class TextProcessor
{
private:
    vector<wstring> textContent;
    wstring line;
    wifstream fin;
    vector<char> x_axis;
    vector<int> y_axis;
    vector<string> labels;
    map<wstring, int> nGramsmap;
    vector<wstring> words;
    wstring word;

public:
    map<string, double> processingTimes;
    map<wchar_t, int> mapCharacter;
    map<wstring, int> mapWord;

    bool readFile(const string &filename)
    {
        locale loc("");
        fin.imbue(loc);
        fin.open(filename);
        if (!fin.is_open())
        {
            wcerr << "Error opening file" << endl;
            return false;
        }
        while (getline(fin, line))
        { // store content
            textContent.push_back(line);
            wcout << line << endl;
        }
        fin.close();
        return true;
    }

    bool applyTransformation()
    {
        locale loc("");

        for (auto &line : textContent)
        {
            transform(line.begin(), line.end(), line.begin(),
                      [&loc](wchar_t c)
                      {
                          return use_facet<ctype<wchar_t>>(loc).tolower(c);
                      });
        }

        for (auto &line : textContent)
        {
            line.erase(remove_if(line.begin(), line.end(),
                                 [](wchar_t c)
                                 { return iswpunct(c); }),
                       line.end());
        }
        return true;
    }

    void printContentInVector() const
    {
        wcout << "\nContents of the file stored in the vector:" << endl;
        for (const auto &line : textContent)
        {
            wcout << line << endl;
        }
    }

    void countCharacterOccurence()
    {
        locale loc("");
        for (const auto &line : textContent)
        {
            for (const auto &character : line)
            {
                if(iswalpha(character)){
                    mapCharacter[character] += 1;
                }
                
            }
            }
    }

    void countWordOccurrence()
    {
        for (const auto &line : textContent)
        {
            wstring word;
            wstringstream wss(line);
            
            while (wss >> word)
            {
                if (mapWord.find(word) != mapWord.end())
                {
                    mapWord[word]++;
                }
                else
                {
                    mapWord[word] = 1;
                }
            }
        }
    }


    template <typename K, typename V>
    
    void printContentInMap(const map<K, V>& map) 
    {
        wcout << "\nContents of the file stored in Map:" << endl;
        for (auto const& [key, val] : map)
        {
            wcout << key << L':' << val << endl;
        }
    }

    template <typename K, typename V>
    void plotFrequencies(const map<K, V>& map, string title, string xlabel) {
        vector<int> x_axis;
        vector<V> y_axis;
        vector<string> labels;

        wstring_convert<codecvt_utf8<wchar_t>, wchar_t> converter; 

        if (title == "Word Frequency" || title.find("gram") != string::npos) {  
            vector<pair<K, V>> top10;

            for (const auto& [key, val] : map) {
                if (top10.size() < 10) {
                    top10.push_back({key, val});
                } else {
                    auto min_it = min_element(top10.begin(), top10.end(), 
                                                [](const auto& a, const auto& b) {
                                                    return a.second < b.second;
                                                });
                    if (val > min_it->second) {
                        *min_it = {key, val};  
                    }
                }
            }

            sort(top10.begin(), top10.end(), 
                    [](const auto& a, const auto& b) {
                        return a.second > b.second;
                    });

            int index = 0;
            for (const auto& [key, val] : top10) {
                string label = converter.to_bytes(key);
                x_axis.push_back(index++);
                y_axis.push_back(val);
                labels.push_back(label);
            }
        } else {
            int index = 0;
            for (const auto& [key, val] : map) {
                string label = converter.to_bytes(key);
                x_axis.push_back(index++);
                y_axis.push_back(val);
                labels.push_back(label);
            }
        }

        plt::figure_size(1200, 800);
        plt::plot(x_axis, y_axis, "*");
        plt::title(title);
        plt::xlabel(xlabel);
        plt::ylabel("Frequency");

        if (x_axis.size() == labels.size()) {
            plt::xticks(x_axis, labels); 
        }

        plt::show();
    }


    void generateNGrams(int n){
        string title = (n == 2) ? "Bigram Frequency"
                            : "Trigram Frequency";
        nGramsmap.clear();
        
        for (const auto& line : textContent)
        {
            wstringstream wss(line);
            vector<wstring> words;

            while (wss >> word) {
                words.push_back(word);
            }

            if(words.size()<n){
                continue;
            }

            for (size_t i = 0; i <= words.size() - n; ++i)
            {
                wstring nGram;

                for (int j = 0; j < n; ++j)
                {
                    if (j > 0) {
                        nGram += L" "; 
                    }
                    nGram += words[i + j];
                }

                nGramsmap[nGram]++;
            }
        }

        wcout << L"\nN-Gram Frequencies (n = " << n << L"):\n";
        for (const auto& [key, val] : nGramsmap)
        {
            wcout << key << L" : " << val << endl;
        }
        plotFrequencies(nGramsmap, title, "N-Grams");
    }

    void measureAndPlotProcessingTime(const string &filename) {
        processingTimes.clear();  // Clear previous measurements

        // Measure readFile
        auto start = high_resolution_clock::now();
        readFile(filename);
        auto end = high_resolution_clock::now();
        processingTimes["readFile"] = duration_cast<milliseconds>(end - start).count();

        // Measure applyTransformation
        start = high_resolution_clock::now();
        applyTransformation();
        end = high_resolution_clock::now();
        processingTimes["applyTransformation"] = duration_cast<milliseconds>(end - start).count();

        // Measure countCharacterOccurrence
        start = high_resolution_clock::now();
        countCharacterOccurence();
        end = high_resolution_clock::now();
        processingTimes["countCharacterOccurrence"] = duration_cast<milliseconds>(end - start).count();

        // Measure countWordOccurrence
        start = high_resolution_clock::now();
        countWordOccurrence();
        end = high_resolution_clock::now();
        processingTimes["countWordOccurrence"] = duration_cast<milliseconds>(end - start).count();

        // Measure generateNGrams for bigrams (n=2)
        start = high_resolution_clock::now();
        generateNGrams(2);
        end = high_resolution_clock::now();
        processingTimes["generateNGrams (2)"] = duration_cast<milliseconds>(end - start).count();

        // Measure generateNGrams for trigrams (n=3)
        start = high_resolution_clock::now();
        generateNGrams(3);
        end = high_resolution_clock::now();
        processingTimes["generateNGrams (3)"] = duration_cast<milliseconds>(end - start).count();

        // Plot the processing times
        plotProcessingTimes();
    }

    void plotProcessingTimes() {
        vector<double> x_axis;   
        vector<double> times;
        vector<string> labels;

        int index = 0;
        for (const auto &[func, time] : processingTimes) {
            labels.push_back(func);
            times.push_back(time);
            x_axis.push_back(static_cast<double>(index++));
        }

        plt::figure_size(1200, 800);
        plt::bar(x_axis, times); 
        plt::title("Processing Time for Each Function");
        plt::xlabel("Function");
        plt::ylabel("Time (ms)");

        plt::xticks(x_axis, labels);
        plt::show();
    }

};

int main()
{
    locale::global(locale(""));
    string filename="textprocessor_files/de/ep-11-11-15-007-14.txt";

    TextProcessor processor;
    if (processor.readFile(filename))
    {
        wcout << "\nFile read successfully!" << endl;
        processor.printContentInVector();
        processor.applyTransformation();
        processor.printContentInVector();
        processor.countCharacterOccurence();
        processor.printContentInMap(processor.mapCharacter);
        processor.countWordOccurrence();
        processor.printContentInMap(processor.mapWord);
        processor.plotFrequencies(processor.mapCharacter, "Character Frequency", "Characters");
        processor.plotFrequencies(processor.mapWord, "Word Frequency", "Words");
        // processor.generateNGrams(2);
        // processor.generateNGrams(3);
        processor.measureAndPlotProcessingTime(filename);
    }
    return 0;
}