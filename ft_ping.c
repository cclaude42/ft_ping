#include "ft_ping.h"

// Info for signals
struct info ginfo = {0};


// In event of Ctrl-C
void interrupt (int sig)
{
    (void)sig;

    printf("\n");
    printf("--- %s ping statistics ---\n", ginfo.dest);
    printf("%u packets transmitted, %u received, %u%% packet loss, time %ums\n",
        ginfo.transmitted, ginfo.received, 100 - (100 * ginfo.received / ginfo.transmitted), timediff(ginfo.start, getnow()) / 1000);

    if (ginfo.received) {
        unsigned int avg = ginfo.sum / ginfo.received;
        unsigned int mdev = sqrt((ginfo.sqsum / ginfo.received) - (avg * avg));

        printf("rtt min/avg/max/mdev = %u.%u/%u.%u/%u.%u/%u.%u ms\n",
            ginfo.min / 1000, ginfo.min % 1000, avg / 1000, avg % 1000,
            ginfo.max / 1000, ginfo.max % 1000, mdev / 1000, mdev % 1000);
    }
    else {
        printf("\n");
    }

    exit(0);
}

// Read echo reply
void recv_reply (int sockfd, struct timeval timesplit, uint16_t seq)
{
    int sent = 1;

    while (sent > 0) {
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
        sent = recvmsg(sockfd, &mhdr, MSG_DONTWAIT);

        struct ppacket reply;
        memcopy(&reply, iov[0].iov_base + IPH_LEN, sizeof(struct ppacket));

        if (sent == IPH_LEN + ICMPH_LEN + DATA_LEN && reply.type == 0 && reply.sequence_number == byteswapped(seq)) {
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &sin.sin_addr, ip, sizeof(ip));

            uint8_t *ttl = iov[0].iov_base + 8;
            unsigned int triptime = timediff(timesplit, getnow());


            printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%d.%d ms\n", sent - IPH_LEN, ip, byteswapped(reply.sequence_number), *ttl, triptime / 1000, triptime % 100);

            ginfo.received++;
            ginfo.min = min(ginfo.min, triptime);
            ginfo.max = max(ginfo.max, triptime);
            ginfo.sum += triptime;
            ginfo.sqsum += triptime * triptime;
        }
    }
}

// Send echo request
void send_request (int sockfd, struct sockaddr_in addr, uint16_t seq)
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

// Get address from ip
void get_addr (char *ip, void *addr)
{
    struct addrinfo *result;

    if (getaddrinfo(ip, NULL, NULL, &result)) {
        fprintf(stderr, "ft_ping: %s: Name or service not known\n", ip);
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
    get_addr(dest, &addr);

    char buf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, buf, sizeof(buf));

    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0) {
        fprintf(stderr, "ft_ping: fatal error: Couldn't open socket\n");
        exit(2);
    }


    printf("PING %s (%s) %d(%d) bytes of data.\n", dest, buf, DATA_LEN, IPH_LEN + ICMPH_LEN + DATA_LEN);

    struct timeval timesplit = getnow();
    for (uint16_t i = 0 ; i < UINT16_MAX ; i++) {
        send_request(sockfd, addr, i+1);
        while (waitsec(timesplit))
            recv_reply(sockfd, timesplit, i+1);
        timesplit = getnow();
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
        fprintf(stderr, "\n");
        fprintf(stderr, "Usage\n");
        fprintf(stderr, "  ft_ping [options] <destination>\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "Options:\n");
        fprintf(stderr, "  <destination>      dns name or ip address\n");
        fprintf(stderr, "  -h                 print help and exit\n");
        fprintf(stderr, "  -v                 verbose output\n");
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
        fprintf(stderr, "ft_ping: usage error: Destination address required\n");
        exit(1);
    }

    return 0;
}