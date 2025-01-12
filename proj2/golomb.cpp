#include <iostream>
#include <cmath>
#include "header/bitStream.h"


class golomb
{
private:
    int m;                // Golomb parameter
    bool useInterleaving; // Encoding mode: true for interleaving, false for sign/magnitude

    // Zigzag mapping: Converts signed integers to non-negative integers
    int zigzagEncode(int x)
    {
        return x >= 0 ? 2 * x : -2 * x - 1;
    }

    // Reverse zigzag mapping
    int zigzagDecode(int z)
    {
        return (z % 2 == 0) ? (z / 2) : -((z + 1) / 2);
    }

public:
    golomb(int m, bool useInterleaving = true) : m(m), useInterleaving(useInterleaving) {}

    void encode(int x, bitStream &bs)
    {
        int mappedValue;
        if (useInterleaving)
        {
            // Apply interleaving to x
            mappedValue = zigzagEncode(x);
        }
        else
        {
            // Apply sign/magnitude to x
            mappedValue = x >= 0 ? x : -x;
            bs.writeBit(x < 0);
        }
        
        int quotient = mappedValue / m;
        int remainder = mappedValue % m;

        // Encode quotient using unary
        for (int i = 0; i < quotient; ++i)
        {
            bs.writeBit(1);
        }
        bs.writeBit(0);

        // Encode remainder
        int b = floor(log2(m));
        int cutoff = (1 << (b + 1)) - m;

        if (cutoff == 1)
        {
            bs.writeBits(remainder, b);
        }
        else
        {
            if (remainder < cutoff)
            {
                bs.writeBits(remainder, b);
            }
            else
            {
                bs.writeBits(remainder + cutoff, b + 1);
            }
        }

        // std::cout << "Encoding Value: " << x
        //           << " (Mapped: " << mappedValue
        //           << "), Quotient: " << quotient
        //           << ", Remainder: " << remainder
        //           << ", b: " << b
        //           << ", cutoff: " << cutoff << std::endl;
    }
    int decode(bitStream &bs)
    {
        // Decode the unary representation of q (count the number of 1 in the beginning of the code)
        int quotient = 0;
        int remainder = 0;
        int b = floor(log2(m));
        int cutoff = (1 << (b + 1)) - m;
        int sign;
        if(!useInterleaving){
            sign = bs.readBit();
            // cout << "sign: " << sign << endl;
        }
            

        while (bs.readBit() == 1)
        {
            quotient++;
            // cout << "incrementing quotient: " << quotient << endl;
        }

        

        int valueBeforeZigzag;
        if (cutoff == 1)
        {
            valueBeforeZigzag = bs.readBits(b);
        }
        else
        {
            remainder = bs.readBits(b);
            // cout << "read remainder: " << remainder << endl;
            if (remainder < cutoff)
            {
                valueBeforeZigzag = remainder;
            }
            else
            {
                int remainder2 = bs.readBits(1);
                valueBeforeZigzag = remainder * 2 + remainder2 - cutoff;
            }
        }
        int decodedValue;
        if (useInterleaving)
        {
            valueBeforeZigzag = (valueBeforeZigzag + quotient * m);
            decodedValue = zigzagDecode(valueBeforeZigzag);
        }
        else
        {
            decodedValue = valueBeforeZigzag + quotient * m ;
            if(sign){
                decodedValue = -decodedValue;
            }
        }

        // cout << "valueBeforeZigzag: " << valueBeforeZigzag << endl;



        // std::cout << "Decoding Value: " << decodedValue
        //           << " (Quotient: " << quotient
        //           << ", Remainder: " << remainder
        //           << ", b: " << b
        //           << ", cutoff: " << cutoff << ")" << std::endl;

        return decodedValue;
    }

    void setM(int newM) {
        m = newM;
    }
};