#include "bitStream.cpp"
#include "golomb.cpp"
#include <cstdint>
#include <cassert>


using namespace std;

void test_zigzag_encoder() {
    golomb g;
    vector<int> values = {-5, -2, 0, 1, 3};
    vector<int> expected = {9, 3, 0, 2, 6};

    cout << "\nTesting Zigzag Encoding...\n";
    for (size_t i = 0; i < values.size(); i++) {
        int encoded = g.zigzag_encode(values[i]);
        assert(encoded == expected[i]);
    }
    cout << "Zigzag Encoding Test Passed!\n";
}

void test_zigzag_decoder() {
    golomb g;
    vector<int> encoded = {9, 3, 0, 2, 6};
    vector<int> expected = {-5, -2, 0, 1, 3};

    cout << "Testing Zigzag Decoding...\n";
    for (size_t i = 0; i < encoded.size(); i++) {
        int decoded = g.zigzag_decode(encoded[i]);
        assert(decoded == expected[i]);
    }
    cout << "Zigzag Decoding Test Passed!\n\n";
}


void test_unary_codec() {
    golomb g;
    bitStream bs;

    // Step 1: Encode the value
    bs.fs.open("unary_test.bin", ios::out | ios::binary | ios::trunc); 
    int pos = 0;
    int q_original = 9;

    cout << "Testing Unary Codec (Encoding + Decoding)...\n";
    cout << "Original value to encode: " << q_original << endl;
    g.unary_encode(bs, q_original, pos); 
    bs.fs.flush();                       
    bs.fs.close();      
    // Step 2: Decode the value
    bs.fs.open("unary_test.bin", ios::in | ios::binary);  

    if (!bs.fs.is_open()) { 
        cout << "ERROR: Unable to open file for reading!" << endl;
        return;  
    }
    
    pos = 0;
    int q_decoded = g.unary_decode(bs, pos); 
    bs.fs.close();                          

    // Step 3: Verify that decoded value matches the original
    cout << "Q Original: " << q_original << ", Q Decoded: " << q_decoded << endl;
    assert(q_original == q_decoded); 
    cout << "Unary Codec Test Passed!\n";
}


int main(){
    cout << "Running tests...\n";
    test_zigzag_encoder();
    test_zigzag_decoder();
    test_unary_codec();
    cout << "All tests passed!\n";
    return 0;
}

