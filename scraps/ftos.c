#include <stdio.h>
#include <stdlib.h>
#include <math.h>

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

    size_t int_digits = 0;
    long long multiplier = 1;
    while (integer_portion /= 10)
    {
        int_digits++;
        multiplier *= 10;
    }

    for (int i = int_digits; i >= 0; i--)
    {
        __to_str_buffer[c++] = '0' + (long long)(x / multiplier) % 10;
        multiplier /= 10;
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

#define FTOS(x)                                     \
    float_to_str(x);                                \
    printf("%f\t%s\n", (double)x, __to_str_buffer); \
                                                    \
    float_to_str(-x);                               \
    printf("%f\t%s\n", -(double)x, __to_str_buffer);

int main()
{
    FTOS(123.456);
    FTOS(123.4);
    FTOS(3.14);
    FTOS(0.001);
    printf("\n");

    FTOS(33);
    FTOS(3);
    FTOS(0);
    printf("\n");

    FTOS(3.1);
    FTOS(3.01);
    FTOS(3.001);
    FTOS(3.0001);
    printf("\n");

    FTOS(10000000000.1);
    // FTOS(1000000000000000000000000000000.1);
    printf("\n");

    printf("Success\n");
    return 0;
}