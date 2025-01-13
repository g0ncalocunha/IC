#include <algorithm>


using namespace std;

class Predictors {
public:
    enum Standards {
        JPEG_PL, //pixel left
        JPEG_PA, //pixel above
        JPEG_PAL, //pixel above left
        JPEG_ABC, //pixel considering three neighbour pixels
        JPEG_MBC, //median between pixel a and b, adjusted by c
        JPEG_LS //format JPEG-LS
    };

    int predictors(Standards standard, int a, int b, int c) {
        switch (standard) {
        case JPEG_PL:
            return a;
        case JPEG_PA:
            return b;
        case JPEG_PAL:
            return c;
        case JPEG_ABC:
            return a + b - c;
        case JPEG_MBC:
            return a + (b - c) / 2;
        case JPEG_LS: {
            int pred = a + b - c;
            int mini = min(a, b);
            int maxi = max(a, b);

            if (c >= maxi) {
                pred = mini;
            } else if (c <= mini) {
                pred = maxi;
            }

            return pred;
        }
        default:
            return 0;
        }
    }
};
