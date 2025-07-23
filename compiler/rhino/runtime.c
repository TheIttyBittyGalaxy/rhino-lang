#include <stdio.h>
#include <stdbool.h>

char __to_str_buffer[64];

// FIXME: This implementation cannot handle particularly large floats
//        e.g. 1000000000000000000000000000000.1
void float_to_str(double x)
{
    size_t c = 0;

    if (x < 0)
    {
        __to_str_buffer[c++] = '-';
        x = -x;
    }

    long long integer_portion = x;
    float rational_portion = x - integer_portion;

    int f = c;
    do
        __to_str_buffer[c++] = '0' + integer_portion % 10;
    while (integer_portion /= 10);
    int l = c - 1;

    while (l > f)
    {
        char t = __to_str_buffer[f];
        __to_str_buffer[f++] = __to_str_buffer[l];
        __to_str_buffer[l--] = t;
    }

    if (rational_portion > 0.0001)
    {
        __to_str_buffer[c++] = '.';
        while (rational_portion > 0.0001)
        {
            int d = rational_portion * 10;
            __to_str_buffer[c++] = '0' + (d % 10);
            rational_portion = rational_portion * 10 - d;
        }
    }

    __to_str_buffer[c] = '\0';
}