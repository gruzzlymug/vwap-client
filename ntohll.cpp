#include <cstdint>

// https://ubuntuforums.org/showthread.php?t=1190710
uint64_t ntohll(const uint64_t value) {
    enum { TYP_INIT, TYP_SMLE, TYP_BIGE };

    union
    {
        uint64_t ull;
        uint8_t  c[8];
    } x;

    // Test if on Big Endian system.
    static int typ = TYP_INIT;

    if (typ == TYP_INIT)
    {
        x.ull = 0x01;
        typ = (x.c[7] == 0x01) ? TYP_BIGE : TYP_SMLE;
    }

    // System is Big Endian; return value as is.
    if (typ == TYP_BIGE)
    {
        return value;
    }

    // else convert value to Big Endian
    x.ull = value;

    int8_t c = 0;
    c = x.c[0]; x.c[0] = x.c[7]; x.c[7] = c;
    c = x.c[1]; x.c[1] = x.c[6]; x.c[6] = c;
    c = x.c[2]; x.c[2] = x.c[5]; x.c[5] = c;
    c = x.c[3]; x.c[3] = x.c[4]; x.c[4] = c;

    return x.ull;
}

uint64_t htonll(const uint64_t value) {
    return ntohll(value);
}
