#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
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
#define CHUNKSIZE 1000 
#define DEBUG 0 
#define MAXLEN 200


int main(int argc, char** argv){
    signal(SIGPIPE, SIG_IGN);
    
    /* Argument Parsing */

    int tag;
    char hostname[MAXLEN];
    char port[MAXLEN];
    memset(hostname, 0, MAXLEN);
    memset(port, 0, MAXLEN);
    unsigned char opcode, shift;

    int flag_h=0;
    int flag_p=0;
    int flag_o=0;
    int flag_s=0;

    while( (tag = getopt(argc, argv, "h:p:o:s:")) != -1) {
        switch(tag){
            case 'h':
                flag_h=1;
                memcpy(hostname, optarg, strlen(optarg));
                break;
            case 'p':
                flag_p=1;
                memcpy(port, optarg, strlen(optarg));
                break;
            case 'o':
                flag_o=1;
                opcode = (unsigned char) atoi(optarg);
                break;
            case 's':
                flag_s=1;
                shift = (unsigned char) atoi(optarg);
                break;
        }
    }
    if( !(flag_h && flag_p && flag_o && flag_s) ){
        if(DEBUG)
            printf("H %d P %d O %d S %d \n", flag_h, flag_p, flag_o, flag_s);
//        printf("Missing options. Terminating.\n");
        exit(-1);
    }
    /* Argument Parsing END */

    // First, make connection to server.
    int server_fd = open_clientfd(hostname, port);
    
    if(server_fd < 0) {
//        printf("Failed to connect to SERVER %s:%s\n", hostname,port);
        return -1;
    }

    rio_t rio;
    rio_readinitb(&rio, server_fd);
    
    // CUT packets by MAXSIZE
    int c = EOF;
    unsigned int i=0;
    unsigned int pack_cnt = 0;
    char buf[CHUNKSIZE];
    char bufout[CHUNKSIZE+8];

    while( (c=getchar()) != EOF ){
        buf[i++] = (char) c;
        if(i == CHUNKSIZE){
            char* packet_sent = malloc(CHUNKSIZE + 8);
            uint32_t len = htonl(CHUNKSIZE+8);

            uint32_t head1 = (opcode) + (shift << 8);
            head1 &= 0xffff;
            uint64_t head_add = ((uint64_t) head1) + (((uint64_t) len)<<32);
            
            memcpy(packet_sent, (char*) &head_add, 8);
            memcpy(packet_sent+8, buf, CHUNKSIZE);
            
            unsigned short chksum = checksum2(packet_sent, CHUNKSIZE+8);

            head1 += (chksum <<16);
            head_add = ((uint64_t) head1) + (((uint64_t) len)<<32);
            
            memcpy(packet_sent, (char*) &head_add, 8);

            rio_writen(server_fd, packet_sent, CHUNKSIZE+8);

            ssize_t rbytes = 0;
            while(rbytes < CHUNKSIZE+8)
                rbytes += rio_read(&rio, bufout+rbytes, CHUNKSIZE+8);
            
            free(packet_sent);
            i=0;
            pack_cnt++;

            unsigned short chk_recv;
            if((chk_recv=checksum2(bufout, CHUNKSIZE+8))){
                exit(-1);
            }
            rio_writen(STDOUT_FILENO, bufout+0x8, CHUNKSIZE);
            memset(bufout,0,CHUNKSIZE+8);
            memset(buf,0,CHUNKSIZE);
            }
    }
    char* packet_sent = malloc(i+8);

    uint32_t len = htonl(i+8);
    uint32_t head1 = opcode + (shift<<8);
    head1 &= 0xffff;
    uint64_t head_add = ((uint64_t) head1) + (((uint64_t) len)<<32);
    memcpy(packet_sent, (char*) &head_add, 8);
    memcpy(packet_sent+8, buf, i);

    unsigned short chksum = checksum2(packet_sent, 8+i);
    head1 += (chksum << 16);
    head_add = ((uint64_t) head1) + (((uint64_t) len)<<32);
    memcpy(packet_sent, (char*) &head_add, 8);

    rio_writen(server_fd, packet_sent, 8+i);
    free(packet_sent);

    ssize_t rbytes = 0;
    while(rbytes < 8+i)
        rbytes += rio_read(&rio, bufout+rbytes, 8+i);
    
    unsigned short chk_recv;

    if((chk_recv=checksum2(bufout, i+8)))
        exit(-1);
    
    rio_writen(STDOUT_FILENO,bufout+8,i);

    close(server_fd);

    return 0;
}


