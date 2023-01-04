
#ifndef FT_PING_H
# define FT_PING_H

// Std
# include <stdlib.h>
# include <unistd.h>
# include <stdio.h>
# include <string.h>

// Time, types, math
# include <sys/time.h>
# include <stdint.h>
# include <math.h>

// Networking
# include <netdb.h>
# include <arpa/inet.h>
# include <linux/if_packet.h>

// Errors & signals
# include <signal.h>
# include <errno.h>


// Global info for signal handling
struct info {
    unsigned int    transmitted;
    unsigned int    received;
    char *          dest;
    struct timeval  start;
    unsigned int    min;
    unsigned int    max;
    unsigned int    sum;
    unsigned int    sqsum;
};


// Packet structure
struct ppacket {
    uint8_t     type;           // 8 for request, 0 for reply
    uint8_t     code;           // Always 0
    uint16_t    checksum;       // Computed total for checks
    uint16_t    identifier;     // Unique ID of current ping "loop" (always 0 here)
    uint16_t    sequence_number;// Unique ID of ping in loop (starts at 1, increments)
    char        time[8];        // Timestamp as data optimization
    char        data[48];       // Data (static)
};

# define REQUEST_TYPE 8
# define REPLY_TYPE 0
# define CODE 0
# define IDENTIFIER 0
# define DATA "0123456789  [ft_ping] ABCDEFGHIJKLMNOPQRSTUVWXYZ"


// Header lengths
# define IPH_LEN 20
# define ICMPH_LEN 8
# define DATA_LEN 56

// Flags
#define FLAG_H 1
#define FLAG_V 2

// Utility
#define UINT16_OVERFLOW 65536;


// Time
struct timeval getnow (void);
unsigned int timediff (struct timeval old, struct timeval new);
struct timeval waitsec (struct timeval timestamp);
void settime (void *time);

// Utils
uint16_t compute_checksum (uint16_t *data, size_t size);
uint16_t byteswapped (uint16_t n);
void setflag (int *flags, char *arg);
void memcopy (void *dst, void *src, size_t n);
unsigned int min (unsigned int a, unsigned int b);
unsigned int max (unsigned int a, unsigned int b);

#endif