#include <cstdint>
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <vector>
// #include "bitStream.cpp"

using namespace std;

class golomb {
private:
    uint32_t m;
    bool zigzag; // If true -> zigzag encoding; else sign and magnitude

public:

    // double calculate_p(const vector<int>& data) {
    //     int count_zeros = 0;
    //     for (int val : data) {
    //         if (val == 0) count_zeros++;
    //     }
    //     return static_cast<double>(count_zeros) / data.size();
    // }

    // void setIdealM(vector<int>& data) {
    //     calculate_p(data);
    //     if (p <= 0 || p >= 1) {
    //         throw invalid_argument("Error: p must be between 0 and 1 (exclusive).");
    //     }
    //     double log2_p = log2(p);
    //     m = ceil(-1 / log2_p);
    //     cout << "Optimal m calculated from p: " << m << endl;
    // }

    void setM(int m_value) {
        if (m_value > 0) {
            m = m_value;
        } else {
            throw invalid_argument("Error: m must be a positive value.");
        }
    }

    void pickOption(bool zigzag) {
        int choice;
        cout << "\nChoose your option:\n";
        cout << "1. Sign and Magnitude\n";
        cout << "2. Positive/Negative Interleaving\n";
        cin >> choice;

        switch (choice) {
            case 1:
                cout << "Selected: Sign and Magnitude\n";
                zigzag = false;
                break;
            case 2:
                cout << "Selected: Positive/Negative Interleaving\n";
                zigzag = true;
                break;
            default:
                cerr << "Invalid choice. Defaulting to Sign and Magnitude.\n";
                zigzag = false;
        }
    }

    int zigzag_encode(int value) const {
        return (value >= 0) ? (value * 2) : (-value * 2 - 1);
    }

    int zigzag_decode(int value) const {
        return (value % 2 == 0) ? (value / 2) : -(value / 2 + 1);
    }

    void unary_encode(bitStream &bs, int q, int &pos)
    {
        for (int i = 0; i < q; i++) {
            bs.writeBit(1, pos);  
            cout << "Encoded: 1, Position: " << pos << endl;  // Debug print
            pos++;  
        }
        bs.writeBit(0, pos);  
        cout << "Encoded: 0, Position: " << pos << endl;  // Debug print
        pos++;
    }


    int unary_decode(bitStream &bs, int &pos) {
        int q = 0;
        
        while (true) {
            uint64_t bits = bs.readBits(pos, 8);  
            
            for (int i = 0; i < 8; i++) {
                int bit = (bits >> (7 - i)) & 1;  
                if (bit == 1) {
                    cout << "Decoded: 1, Position: " << pos << endl; 
                    q++; 
                } else {
                    cout << "Decoded: 0, Position: " << pos << endl; 
                    pos += i + 1; 
                    return q;  
                }
            }
            pos += 8;
        }
    }


    void binary_encode(bitStream &bs, int r, int b_bits, int &pos) {
        for (int i = b_bits - 1; i >= 0; i--) {
            bs.writeBit((r >> i) & 1, pos++);
        }
    }

    int binary_decode(bitStream &bs, int b_bits, int &pos) {
        int r = 0;
        for (int i = 0; i < b_bits; i++) {
            r = (r << 1) | bs.readBit(pos++);
        }
        return r;
    }

    void encodeGolomb(int value, bitStream &bs, int &pos) {
        int encoded_value = zigzag ? zigzag_encode(value) : abs(value);
        if (!zigzag && value < 0) {
            bs.writeBit(1, pos++); // Write sign bit for negative numbers
        } else if (!zigzag) {
            bs.writeBit(0, pos++); // Write sign bit for positive numbers
        }

        int quotient = encoded_value / m;
        int remainder = encoded_value % m;
        int b_bits = ceil(log2(m));

        unary_encode(bs, quotient, pos);
        binary_encode(bs, remainder, b_bits, pos);
    }

    int decodeGolomb(bitStream &bs, int &pos) {
        bool is_negative = false;
        if (!zigzag) {
            is_negative = bs.readBit(pos++);
        }

        int quotient = unary_decode(bs, pos);
        int b_bits = ceil(log2(m));
        int remainder = binary_decode(bs, b_bits, pos);

        int encoded_value = quotient * m + remainder;

        if (zigzag) {
            return zigzag_decode(encoded_value);
        } else {
            return is_negative ? -encoded_value : encoded_value;
        }
    }
};

// int main() {
//     golomb g;
//     bitStream bs;

//     try {
//         int choice;
//         cout << "Enter a value for m: ";
//         cin >> m_value;
//         g.setM(m_value);

//         if(m_value<0){
//             g.pickOption(); 
//         }
//         else{

//         }

//         bs.openFile("encoded_data.bin");

//         int pos = 0;
//         int values[] = {5, -3, 7, -8};
//         cout << "Encoding values: ";
//         for (int value : values) {
//             cout << value << " ";
//             g.encodeGolomb(value, bs, pos);
//         }
//         cout << "\nEncoding complete. Data written to file.\n";

//         bs.fs.close();
//         bs.openFile("encoded_data.bin");

//         pos = 0;
//         cout << "Decoding values: ";
//         for (size_t i = 0; i < 4; i++) {
//             int decoded_value = g.decodeGolomb(bs, pos);
//             cout << decoded_value << " ";
//         }
//         cout << "\nDecoding complete.\n";

//     } catch (const exception &e) {
//         cerr << e.what() << endl;
//     }

//     return 0;
// }

