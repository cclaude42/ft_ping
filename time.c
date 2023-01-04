#include "ft_ping.h"


// Return timeval struct with current time
struct timeval getnow (void)
{
    struct timeval now;

    gettimeofday(&now, 0);

    return now;
}

// Return usec diff between two timeval structs
unsigned int timediff (struct timeval old, struct timeval new)
{
    return (new.tv_sec - old.tv_sec) * 1000000 + new.tv_usec - old.tv_usec;
}

// Wait til timestamp + 1 second
struct timeval waitsec (struct timeval timestamp)
{
    while (timediff(timestamp, getnow()) < 1000000)
        ;

    return getnow();
}

// Write timestamp to memory (packet)
void settime (void *time)
{
    struct timeval ptr;

    gettimeofday(&ptr, 0);

    memcopy(time, &ptr.tv_sec, 4);
}
