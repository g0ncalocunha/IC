#include <string>
#include <fstream>
#include <vector>
#include <iostream>
#include <algorithm> 
#include <clocale>
#include <cwctype>


using namespace std;

class TextProcessor{
    private: 
        vector<wstring> textContent;
        wstring line;
        wifstream fin;

    public:
        bool readFile(const string& filename){
            locale loc("");
            fin.imbue(loc);
            fin.open(filename);
            if(!fin.is_open()){
                wcerr << "Error opening file" << endl;
                return false;
            }
            while (getline(fin, line)) {  // store content
               textContent.push_back(line);
               wcout << line << endl;
            }
            fin.close();
            return true;
        }

        bool applyTransformation(){
            locale loc(""); 

            for (auto& line : textContent) {
                transform(line.begin(), line.end(), line.begin(),
                    [&loc](wchar_t c) { 
                        return use_facet<ctype<wchar_t>>(loc).tolower(c);
                    }
                );
            }

            for ( auto& line: textContent){
                line.erase(remove_if(line.begin(), line.end(),
                                    [](wchar_t c) { return iswpunct(c); }),
                            line.end());
            }
            return true;
        }

        void printContentInVector() const {
            wcout << "\nContents of the file stored in the vector:" << endl;
            for (const auto& line : textContent) {
                wcout << line << endl; 
            }
        }

};
int main(){
        locale::global(locale(""));

        TextProcessor processor;
        if (processor.readFile("a.txt")) {
            wcout << "\nFile read successfully!" << endl;
        }
        processor.printContentInVector();
        processor.applyTransformation();
        processor.printContentInVector();
        return 0;
}