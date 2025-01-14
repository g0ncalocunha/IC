#include "golomb.h"
#include <cmath>

golomb::golomb(int m, bool useInterleaving) : m(m), useInterleaving(useInterleaving) {}

int golomb::zigzagEncode(int x) {
    return x >= 0 ? 2 * x : -2 * x - 1;
}

int golomb::zigzagDecode(int z) {
    return (z % 2 == 0) ? (z / 2) : -((z + 1) / 2);
}

void golomb::encode(int x, bitStream &bs) {
    int mappedValue;
    if (useInterleaving) {
        mappedValue = zigzagEncode(x);
    } else {
        mappedValue = x >= 0 ? x : -x;
        bs.writeBit(x < 0);
    }
    
    int quotient = mappedValue / m;
    int remainder = mappedValue % m;

    for (int i = 0; i < quotient; ++i) {
        bs.writeBit(1);
    }
    bs.writeBit(0);

    int b = floor(log2(m));
    int cutoff = (1 << (b + 1)) - m;

    if (cutoff == 1) {
        bs.writeBits(remainder, b);
    } else {
        if (remainder < cutoff) {
            bs.writeBits(remainder, b);
        } else {
            bs.writeBits(remainder + cutoff, b + 1);
        }
    }
}

int golomb::decode(bitStream &bs) {
    int quotient = 0;
    int remainder = 0;
    int b = floor(log2(m));
    int cutoff = (1 << (b + 1)) - m;
    int sign;
    if(!useInterleaving){
        sign = bs.readBit();
    }

    while (bs.readBit() == 1) {
        quotient++;
    }

    int valueBeforeZigzag;
    if (cutoff == 1) {
        valueBeforeZigzag = bs.readBits(b);
    } else {
        remainder = bs.readBits(b);
        if (remainder < cutoff) {
            valueBeforeZigzag = remainder;
        } else {
            int remainder2 = bs.readBits(1);
            valueBeforeZigzag = remainder * 2 + remainder2 - cutoff;
        }
    }
    int decodedValue;
    if (useInterleaving) {
        valueBeforeZigzag = (valueBeforeZigzag + quotient * m);
        decodedValue = zigzagDecode(valueBeforeZigzag);
    } else {
        decodedValue = valueBeforeZigzag + quotient * m ;
        if(sign){
            decodedValue = -decodedValue;
        }
    }

    return decodedValue;
}

void golomb::setM(int newM) {
    m = newM;
}