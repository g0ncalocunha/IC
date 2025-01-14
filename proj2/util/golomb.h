#ifndef GOLOMB_H
#define GOLOMB_H

#include "bitStream.h"

class golomb {
private:
    int m;                // Golomb parameter
    bool useInterleaving; // Encoding mode: true for interleaving, false for sign/magnitude

    // Zigzag mapping: Converts signed integers to non-negative integers
    int zigzagEncode(int x);

    // Reverse zigzag mapping
    int zigzagDecode(int z);

public:
    golomb(int m, bool useInterleaving = true);

    void encode(int x, bitStream &bs);
    int decode(bitStream &bs);

    void setM(int newM);
};

#endif // GOLOMB_H