#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include "network.h"

#define SERVER "143.248.56.16"
#define PORT "5000"

unsigned short checksum1(const char *buf, unsigned size)
{
    unsigned sum = 0;
    int i;

    /* Accumulate checksum */
    for (i = 0; i < size - 1; i += 2)
    {
        unsigned short word16 = *(unsigned short *) &buf[i];
        sum += word16;
    }

    /* Handle odd-sized case */
    if (size & 1)
    {
        unsigned short word16 = (unsigned char) buf[i];
        sum += word16;
    }

    /* Fold to get the ones-complement result */
    while (sum >> 16) sum = (sum & 0xFFFF)+(sum >> 16);

    /* Invert to get the negative in ones-complement arithmetic */
    return ~sum;
}


int main(int argc, char** argv){

    signal(SIGPIPE, SIG_IGN);

    // First, make connection to server.
    int server_fd = open_clientfd(SERVER, PORT);

    char buf[10];

    unsigned char opt = 0;
    unsigned char n = 2;
    unsigned int chksum;

    unsigned long lengthbytes;

    while(1){
        int idx=0;
        chksum = 0;
        scanf("%s",buf);

        for(idx=0; idx<10; idx++){
            chksum += (unsigned int) buf[idx];
        }

        chksum += (n+opt);
        chksum = ~chksum;

        printf("chksum value = 0x%x\n",chksum);
        assert(checksum1(buf,10) == checksum2(buf,10));

    }

    return 0;
}


