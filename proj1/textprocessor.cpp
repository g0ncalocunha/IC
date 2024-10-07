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

namespace plt=matplotlibcpp;

using namespace std;

class TextProcessor
{
private:
    vector<wstring> textContent;
    wstring line;
    wifstream fin;
    vector<char> x_axis;
    vector<int> y_axis;
    vector<string> labels;

public:
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

    template <typename K, typename V>
    
    void printContentInMap(const map<K, V>& map) 
    {
        wcout << "\nContents of the file stored in Map:" << endl;
        for (auto const& [key, val] : map)
        {
            wcout << key << L':' << val << endl;
        }
    }

    void countWordOccurence()
    {
        locale loc("");
        for (const auto &line : textContent)
        {
            for (const auto &word : line)
            {
                mapWord[word] += 1;
            }
        }
    }

    template <typename K, typename V>
    void plotFrequencies(const map<K, V>& map, string title, string xlabel) {
        x_axis.clear();
        y_axis.clear();
        labels.clear();  

        int index = 0;
        wstring_convert<codecvt_utf8<wchar_t>, wchar_t> converter; 

        for (auto const& [key, val] : map) {
            string label;

            label = converter.to_bytes(key);

            x_axis.push_back(index++);
            y_axis.push_back(val);
            labels.push_back(label);  
        }

        plt::plot(x_axis, y_axis, "*");
        plt::title(title);
        plt::xlabel(xlabel);
        plt::ylabel("Frequency");

        if (x_axis.size() == labels.size()) {
            plt::xticks(x_axis, labels); 
        }

        plt::show();
    }


};

int main()
{
    locale::global(locale(""));

    TextProcessor processor;
    if (processor.readFile("a.txt"))
    {
        wcout << "\nFile read successfully!" << endl;
    }
    processor.printContentInVector();
    processor.applyTransformation();
    processor.printContentInVector();
    processor.countCharacterOccurence();
    processor.printContentInMap(processor.mapCharacter);
    processor.plotFrequencies(processor.mapCharacter, "Character Frequency", "Characters");
    return 0;
}