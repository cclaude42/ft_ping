#include "ft_ping.h"


// Returns the checksum of the packet read
uint16_t compute_checksum (uint16_t *data, size_t size)
{
    unsigned long sum = 0;

    for (size_t i = 0 ; i < size / 2 ; i++)
        sum += data[i];

    uint16_t chksm = sum + sum / UINT16_OVERFLOW;

    return ~chksm;
}

// Returns a uint16_t with its two bytes swapped
uint16_t byteswapped (uint16_t n)
{
    return (n << 8) + (n >> 8);
}

// Sets flags when parsing args
void setflag (int *flags, char *arg)
{
    for (int i = 1 ; arg[i] ; i++) {
        if (arg[i] == 'h')
            *flags |= FLAG_H;
        else if (arg[i] == 'v')
            *flags |= FLAG_V;
    }
}

// Analogous to memcpy
void memcopy (void *dst, void *src, size_t n)
{
    for (size_t i = 0 ; i < n ; i++)
        ((char *)dst)[i] = ((char *)src)[i];
}

// Analogous to min
unsigned int min (unsigned int a, unsigned int b)
{
    if (a < b)
        return a;
    else
        return b;
}

// Analogous to max
unsigned int max (unsigned int a, unsigned int b)
{
    if (a > b)
        return a;
    else
        return b;
}