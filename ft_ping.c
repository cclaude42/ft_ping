

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>

#include <errno.h>

    //    #include <sys/socket.h>
       
    //    #include <net/ethernet.h> /* the L2 protocols */

    //    #include <sys/types.h>
    //    #include <sys/socket.h>
#include <netdb.h>


struct ppacket {
    uint8_t     type;           // 8 for request, 0 for reply
    uint8_t     code;           // Always 0 for requests and replies
    uint16_t    checksum;       // Computed total for checks
    uint16_t    identifier;     // Unique ID of current ping "loop" (starts at 1, increments)
    uint16_t    sequence_number;// Unique ID of (starts at 1, increments)
    char        time[8];
    char        data[48];
};


#define UINT16_OVERFLOW 65536;

#define FLAG_H 1
#define FLAG_V 2

#define TYPE 8
#define CODE 0
#define DATA "0123456789  [ft_ping] ABCDEFGHIJKLMNOPQRSTUVWXYZ"


#define IPH_LEN 20
#define ICMPH_LEN 8
#define DATA_LEN 56

uint16_t compute_checksum (uint16_t *data, size_t size)
{
    unsigned long sum = 0;

    for (int i = 0 ; i < size / 2 ; i++)
        sum += data[i];

    uint16_t chksm = sum + sum / UINT16_OVERFLOW;

    return ~chksm;
}

unsigned int timediff (struct timeval old, struct timeval new)
{
    return (new.tv_sec - old.tv_sec) * 1000000 + new.tv_usec - old.tv_usec;
}

struct timeval getnow (void)
{
    struct timeval now;
    gettimeofday(&now, 0);
    return now;
}

struct timeval wait_sec (struct timeval last)
{
    while (timediff(last, getnow()) < 1000000)
        ;

    return getnow();
}


void settime (void *time)
{
    struct timeval ptr;

    gettimeofday(&ptr, 0);

    memcpy(time, &ptr.tv_sec, 4);
}


uint16_t byteswapped (uint16_t n)
{
    return (n << 8) + (n >> 8);
}


void send_request (int sockfd, struct sockaddr_in addr, unsigned int seq)
{
    struct ppacket request = {0};
    request.type = TYPE;
    request.code = CODE;
    request.identifier = 0;
    request.sequence_number = byteswapped(seq);
    settime(&request.time);
    memcpy(&request.data, DATA, sizeof(DATA));
    request.checksum = compute_checksum((uint16_t *)&request, sizeof(request));

    int ret = sendto(sockfd, &request, 64, 0, (struct sockaddr *)&addr, sizeof(addr));
}


void watch_reply (int sockfd, struct timeval timesplit)
{
    char buf[1000] = {0};
    char control[1000] = {0};

    struct sockaddr_in sin = {0};

    struct iovec iov[1];
    iov[0].iov_base = buf;
    iov[0].iov_len = 1000;

    struct msghdr mhdr;
    mhdr.msg_iov = iov;
    mhdr.msg_iovlen = 1;
    mhdr.msg_name = &sin;
    mhdr.msg_namelen = sizeof(sin);
    mhdr.msg_control = &control;
    mhdr.msg_controllen = sizeof(control);


    int ret = recvmsg(sockfd, &mhdr, 0);

    if (ret > 0) {
        char buf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &sin.sin_addr, buf, sizeof(buf));

        uint16_t *seq = iov[0].iov_base + IPH_LEN + 6;
        uint8_t *ttl = iov[0].iov_base + 8;
        unsigned int time = timediff(timesplit, getnow());

        printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%d.%d ms\n", ret - IPH_LEN, buf, byteswapped(*seq), *ttl, time / 1000, time % 100);
    }
}


void setflag (int *flags, char *arg)
{
    for (int i = 1 ; arg[i] ; i++) {
        if (arg[i] == 'h')
            *flags |= FLAG_H;
        else if (arg[i] == 'v')
            *flags |= FLAG_V;
    }
}


struct sockaddr_in * check_addr (char *ip, void *addr)
{
    struct addrinfo *result;

    if (getaddrinfo(ip, NULL, NULL, &result)) {
        printf("ft_ping: %s: Name or service not known\n", ip);
        exit(2);
    }
    
    for (struct addrinfo *node = result ; node && node->ai_next ; node = node->ai_next) {
        if (((struct sockaddr_in *)node->ai_addr)->sin_addr.s_addr) {
            memcpy(addr, node->ai_addr, sizeof(struct sockaddr_in));
            break ;
        }
    }

    freeaddrinfo(result);
}



int main (int ac, char **av)
{
    int dest = 0;
    int flags = 0;

    for (int i = 1 ;  i < ac ; i++) {
        if (av[i][0] != '-')
            dest = i;
        else
            setflag(&flags, av[i]);
    }

    if (flags & FLAG_H) {
        printf("\n");
        printf("Usage\n");
        printf("  ft_ping [options] <destination>\n");
        printf("\n");
        printf("Options:\n");
        printf("  <destination>      dns name or ip address\n");
        printf("  -h                 print help and exit\n");
        printf("  -v                 verbose output\n");
    }
    else if (dest) {
        struct sockaddr_in addr = {0};
        check_addr(av[dest], &addr);
    
        char buf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr.sin_addr, buf, sizeof(buf));

        printf("PING %s (%s) %d(%d) bytes of data.\n", av[dest], buf, DATA_LEN, IPH_LEN + ICMPH_LEN + DATA_LEN);

        int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

        struct timeval timesplit = getnow();
        for (int i = 0 ; i < 256 ; i++) {
            send_request(sockfd, addr, i+1);
            watch_reply(sockfd, timesplit);
            timesplit = wait_sec(timesplit);
        }
    }
    else {
        printf("ft_ping: usage error: Destination address required\n");
    }

    return 0;
}