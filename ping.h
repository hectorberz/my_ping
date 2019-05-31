
#ifndef PING_H
#define PING_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <netinet/ip.h>      //ip hdeader library (must come before ip_icmp.h)
#include <netinet/ip_icmp.h> //icmp header
#include <arpa/inet.h>       //internet address library
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>

#include "checksum.h" //my checksum library

#define ICMP_LEN 28
#define PROT_ICMP 1
#define BUFSIZE 1028
#define DATALEN 56
#define MAX_LONG __LONG_MAX__

static long npings = 0;
static long pings_repeated = 0;
static long pings_received = 0;

static long trip_time = 0;
static unsigned long total_time = 0;
static unsigned long total_time_2 = 0;
static long trip_time_min = MAX_LONG;
static long trip_time_max = 0;
struct timeval time_val;
struct timeval time_out;

struct sockaddr_in who_tp;
static int soc_fd;
char sendbuf[BUFSIZE], recvbuf[BUFSIZE], controlbuf[BUFSIZE / 8];
char *hostname, *targ, nbuf[64];
struct icmp *icmp;
struct ip *ip;
static int packet_len, recv_len, ip_len;
static int data_len = DATALEN;
struct addrinfo *ai;
struct iovec iov;
struct msghdr msg;
struct timeval royal_time;
static int ident;
static volatile int loop;

void get_time(struct timeval *start, struct timeval *end);
void exit_handler(int signum);
void send_echo();
void receive_echo();
void timeout_handler(int signum);
void packet_p();
void *thread_handler(void *ptr);
unsigned long isqrt(unsigned long x);
#endif /* PING_H */
