#include <cctype>
#include <algorithm>
#include <clocale>
#include <iostream>
#include <fstream>
#include <string>

using namespace std;

int main(int argc, char* argv[]) {
  ifstream fin;
  string line;
  string file = argv[1];
  fin.open(file);
  while(getline(fin, line)){
    transform(line.begin(), line.end(), line.begin(),[](unsigned char c){ return tolower(c);});
    cout << line << endl;
  }
  return 0;
}