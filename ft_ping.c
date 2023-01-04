#include "ft_ping.h"

// Info for signals
struct info ginfo = {0};


// In event of Ctrl-C
void interrupt (int sig)
{
    (void)sig;
    unsigned int avg = ginfo.sum / ginfo.received;
    unsigned int mdev = sqrt((ginfo.sqsum / ginfo.received) - (avg * avg));

    printf("\n");
    printf("--- %s ping statistics ---\n", ginfo.dest);
    printf("%u packets transmitted, %u received, %u%% packet loss, time %ums\n",
        ginfo.transmitted, ginfo.received, 100 - (100 * ginfo.received / ginfo.transmitted), timediff(ginfo.start, getnow()) / 1000);
    printf("rtt min/avg/max/mdev = %u.%u/%u.%u/%u.%u/%u.%u ms\n",
        ginfo.min / 1000, ginfo.min % 1000, avg / 1000, avg % 1000,
        ginfo.max / 1000, ginfo.max % 1000, mdev / 1000, mdev % 1000);
    exit(0);
}

// Send echo request
void send_request (int sockfd, struct sockaddr_in addr, unsigned int seq)
{
    // Init packet
    struct ppacket request = {0};

    request.type = REQUEST_TYPE;
    request.code = CODE;
    request.identifier = IDENTIFIER;
    request.sequence_number = byteswapped(seq);
    settime(&request.time);
    memcopy(&request.data, DATA, sizeof(DATA));
    request.checksum = compute_checksum((uint16_t *)&request, sizeof(request));


    // Send packet
    int ret = sendto(sockfd, &request, 64, 0, (struct sockaddr *)&addr, sizeof(addr));

    if (ret > 0)
        ginfo.transmitted++;
}

// Read echo reply
void recv_reply (int sockfd, struct timeval timesplit)
{
    // Init msghdr
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


    // Read packet
    int ret = recvmsg(sockfd, &mhdr, 0);

    if (ret > 0) {
        char buf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &sin.sin_addr, buf, sizeof(buf));

        uint16_t *seq = iov[0].iov_base + IPH_LEN + 6;
        uint8_t *ttl = iov[0].iov_base + 8;
        unsigned int triptime = timediff(timesplit, getnow());

        printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%d.%d ms\n", ret - IPH_LEN, buf, byteswapped(*seq), *ttl, triptime / 1000, triptime % 100);
        
        ginfo.received++;
        ginfo.min = min(ginfo.min, triptime);
        ginfo.max = max(ginfo.max, triptime);
        ginfo.sum += triptime;
        ginfo.sqsum += triptime * triptime;
    }
}

// Get address from ip
void check_addr (char *ip, void *addr)
{
    struct addrinfo *result;

    if (getaddrinfo(ip, NULL, NULL, &result)) {
        printf("ft_ping: %s: Name or service not known\n", ip);
        exit(2);
    }
    
    for (struct addrinfo *node = result ; node && node->ai_next ; node = node->ai_next) {
        if (((struct sockaddr_in *)node->ai_addr)->sin_addr.s_addr) {
            memcopy(addr, node->ai_addr, sizeof(struct sockaddr_in));
            break ;
        }
    }

    freeaddrinfo(result);
}

// Ping
void ping (char *dest)
{
    struct sockaddr_in addr = {0};
    check_addr(dest, &addr);

    char buf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, buf, sizeof(buf));

    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0) {
        printf("ft_ping: fatal error: Couldn't open socket\n");
        exit(2);
    }


    printf("PING %s (%s) %d(%d) bytes of data.\n", dest, buf, DATA_LEN, IPH_LEN + ICMPH_LEN + DATA_LEN);

    struct timeval timesplit = getnow();
    for (int i = 0 ; i < 256 ; i++) {
        send_request(sockfd, addr, i+1);
        recv_reply(sockfd, timesplit);
        timesplit = waitsec(timesplit);
    }
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
        exit(0);
    }
    else if (dest) {
        ginfo.dest = av[dest];
        ginfo.start = getnow();
        ginfo.min = 100000;
        signal(SIGINT, interrupt);

        ping(av[dest]);
    }
    else {
        printf("ft_ping: usage error: Destination address required\n");
        exit(1);
    }

    return 0;
}