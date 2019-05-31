
/*
 *  ping.c  
 *  by Zachary Gilbert
 *  ICS 451
 *  My personal pingish implementation
 *  
 *  Globals are located in ping.h
*/

#include "ping.h"

int main(int argc, char *argv[])
{
    struct sockaddr_in *to;
    struct hostent *hent;

    signal(SIGINT, exit_handler);
    gettimeofday(&royal_time, (struct timezone *)NULL);
    loop = 1;

    ident = getpid() & 0xFFFF;

    if (getaddrinfo(argv[1], NULL, NULL, &ai) != 0)
    {
        perror("PING-ERROR: Address Info");
        exit(1);
    }

    if ((soc_fd = socket(AF_INET, SOCK_RAW, PROT_ICMP)) < 0)
    {
        if (errno == EPERM)
        {
            fprintf(stderr, "PING: raw sockets require root privileges\n");
        }
        else
        {
            perror("PING: socket");
        }
        exit(1);
    }
    targ = argv[1];

    setsockopt(soc_fd, SOL_SOCKET, SO_RCVTIMEO, &time_out, sizeof(struct timeval));

    to = (struct sockaddr_in *)&who_tp;
    to->sin_family = AF_INET;

    if (!inet_aton(targ, &to->sin_addr))
    {
        hent = gethostbyname(targ);
        to->sin_family = hent->h_addrtype;
        if (hent->h_length > (int)sizeof(to->sin_addr))
        {
            hent->h_length = sizeof(to->sin_addr);
        }
        memcpy(&to->sin_addr, hent->h_addr, hent->h_length);
        strncpy(nbuf, hent->h_name, sizeof(nbuf) - 1);
        hostname = nbuf;
    }
    else
    {
        hostname = targ;
    }

    if (to->sin_family == AF_INET)
    {
        printf("PING: %s (%s): %d data bytes\n", hostname, inet_ntoa(to->sin_addr), data_len);
    }
    else
    {
        printf("PING: %s: %d data bytes\n", hostname, data_len);
    }

    while (1)
    {
        if (loop)
        {
            loop = 0;
            send_echo();
            receive_echo();

            while (!loop)
                ;
            packet_p();
        }
    }
    return 0;
}

/*
 * send_echo
 * 
 * sends and imcp echo message.
 */
void send_echo()
/**
        int var = 20;
        setsockopt(soc_fd, IPPROTO_RAW, IP_TTL, (char *)&var, sizeof(var));
*/
{
    gettimeofday((struct timeval *)&sendbuf[8], (struct timezone *)NULL);
    icmp = (struct icmp *)sendbuf;
    icmp->icmp_type = ICMP_ECHO;
    icmp->icmp_code = 0;
    icmp->icmp_id = ident;
    icmp->icmp_seq = ++npings;
    icmp->icmp_cksum = 0;

    packet_len = data_len + 8;

    icmp->icmp_cksum = checksum((unsigned short *)icmp, packet_len);
    if ((sendto(soc_fd, sendbuf, packet_len, 0, (struct sockaddr *)&who_tp, sizeof(struct sockaddr))) < 0)

    {
        perror("PING: sendto");
        exit(1);
    }
}

/*
 * receive_echo
 * 
 * Receives and icmp echo message.
 */
void receive_echo()
{
    iov.iov_base = recvbuf;
    iov.iov_len = BUFSIZE;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = controlbuf;
    msg.msg_controllen = BUFSIZE;

    recv_len = recvmsg(soc_fd, &msg, 0);
    gettimeofday(&time_val, (struct timezone *)NULL);
    if (recv_len > -1)
    {
        sleep(1);
    }
    loop = 1;
}

/*
 * packet_p
 * 
 * prints the packet information, such that is has a pingish tone to it.
 */
void packet_p()
{
    struct timeval *time_ptr;
    trip_time = 0;

    ip = (struct ip *)recvbuf;
    ip_len = ip->ip_hl << 2;
    recv_len -= ip_len;
    icmp = (struct icmp *)(recvbuf + ip_len);

    if (icmp->icmp_type == ICMP_ECHOREPLY)
    {
        if (icmp->icmp_id != ident)
        {
            if (npings > 10)
                exit_handler(0);
            return;
        }
        time_ptr = (struct timeval *)icmp->icmp_data;
        get_time(&time_val, time_ptr);
        trip_time = time_val.tv_sec * 10000 + (time_val.tv_usec / 100);
        total_time += trip_time;
        total_time_2 += (long)trip_time * (long)trip_time;

        if (trip_time < trip_time_min)
        {
            trip_time_min = trip_time;
        }
        else if (trip_time > trip_time_max)
        {
            trip_time_max = trip_time;
        }
        pings_received++;
    }
    printf("%d bytes from %s (%s): icmp_seq=%u ttl=%d time=%ld.%ld ms\n",
           recv_len, hostname, inet_ntoa(ip->ip_src), icmp->icmp_seq,
           ip->ip_ttl, trip_time / 10, trip_time % 10);
    printf("icmp type: %d\n", icmp->icmp_type);
}

/*
 * exit_handler
 * 
 * prints the exit message when SIDINT is activated or when ping failed to receive ten 
 * echo responses.
 */
void exit_handler(int signum)
{
    struct timeval royal_time_fin;
    long royal_time_val;
    int pl = (int)((npings - pings_received) * 100 / npings);
    unsigned long total_time_av;
    unsigned long total_time_2_av;
    unsigned long mdev;

    gettimeofday(&royal_time_fin, (struct timezone *)NULL);
    get_time(&royal_time_fin, &royal_time);
    royal_time_val = royal_time_fin.tv_sec * 10000 + (royal_time_fin.tv_usec / 100);

    if (pings_received < npings && recv_len == -1)
    {
        printf("\n--- %s PING statistics ---\n", hostname);
        printf("%ld packets transmitted, %ld received, %d%% packet loss, time %ldms\n",
               npings, pings_received, pl, royal_time_val / 10);
        close(soc_fd);
        exit(1);
    }
    total_time_av = total_time / (pings_received + pings_repeated);
    total_time_2_av = total_time_2 / (pings_received + pings_repeated);
    mdev = isqrt(total_time_2_av - (total_time_av * total_time_av));

    printf("\n--- %s PING statistics ---\n", hostname);
    printf("%ld packets transmitted, %ld received, %d%% packet loss, time %ldms\n",
           npings, pings_received, pl, royal_time_val / 10);
    printf("rtt min/avg/max/mdev = %ld.%ld/%ld.%ld/%ld.%ld/%ld.%ld ms\n",
           trip_time_min / 10, trip_time_min % 10,
           (total_time_av) / 10, (total_time_av) % 10,
           trip_time_max / 10, trip_time_max % 10,
           mdev / 10, mdev % 10);

    close(soc_fd);
    exit(0);
}

/*
 * isgrt 
 * 
 * Gets the square root of an unsigned long type.
 * Source: http://www.codecodex.com/wiki/Calculate_an_integer_square_root
 */
unsigned long isqrt(unsigned long x)
{
    register unsigned long op, res, one;

    op = x;
    res = 0;

    one = 1 << 30;
    while (one > op)
        one >>= 2;

    while (one != 0)
    {
        if (op >= res + one)
        {
            op -= res + one;
            res += one << 1;
        }
        res >>= 1;
        one >>= 2;
    }
    return res;
}

/*
 * get_time
 * 
 * Computes the time in microseconds between two timeval objects
 */
void get_time(struct timeval *res, struct timeval *ptr)
{
    if ((res->tv_usec -= ptr->tv_usec) < 0)
    {
        res->tv_sec--;
        res->tv_usec += 1000000;
    }
    res->tv_sec -= ptr->tv_sec;
}
