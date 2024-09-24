#include <string>
#include <fstream>
#include <vector>
#include <iostream>
#include <cctype>
#include <algorithm> 
#include <clocale>


using namespace std;

class TextProcessor{
    private: 
        vector<string> textContent;
        string line;
        ifstream fin;

    public:
        bool readFile(const string& filename){
            fin.open(filename);
            if(!fin.is_open()){
                cerr << "Error opening file" << endl;
                return false;
            }
            while (getline(fin, line)) {  // store content
               textContent.push_back(line);
               cout << line << endl;
            }
            fin.close();
            return true;
        }

        bool applyTransformation(){

            for (auto& line : textContent) {
                transform(line.begin(), line.end(), line.begin(),
                    [](unsigned char c){ 
                        return tolower(c);
                    }
                );
            }

            for ( auto& line: textContent){
                line.erase(remove_if(line.begin(), line.end(),
                                    [](unsigned char c) { return ispunct(c); }),
                            line.end());
            }
            return true;
        }

        void printContentInVector() const {
            cout << "\nContents of the file stored in the vector:" << endl;
            for (const auto& line : textContent) {
                cout << line << endl; 
            }
        }

};
int main(){
        TextProcessor processor;
        if (processor.readFile("a.txt")) {
            cout << "\nFile read successfully!" << endl;
        }
        processor.printContentInVector();
        processor.applyTransformation();
        processor.printContentInVector();
        return 0;
}