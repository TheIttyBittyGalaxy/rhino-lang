#include <stdint.h>

#define GENERAL(R)                \
    union                         \
    {                             \
        uint32_t E##R##X;         \
        struct                    \
        {                         \
            uint16_t __##R;       \
            union                 \
            {                     \
                uint16_t R##X;    \
                union             \
                {                 \
                    uint8_t R##H; \
                    uint8_t R##L; \
                };                \
            };                    \
        };                        \
    }

typedef struct
{
    GENERAL(A);
    GENERAL(B);
    GENERAL(C);
    GENERAL(D);
    uint32_t ESI;
    uint32_t EDI;
    uint32_t ESP; // Stack pointer
    uint32_t EBP; // Base pointer
} Registers;

#undef GENERAL