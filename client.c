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
#define CHUNKSIZE 32 
#define DEBUG 0
#define MAXLEN 200

#define HEADER(op,n,chksum,len) ((((htons(op)<<8) + htons(n))<<48) + (htons(chksum)<<32) + htonl(len))

void sigpipe_handler(int sig){
    printf("[CHECKSUM] Validation failed.\n");
}


int main(int argc, char** argv){
    signal(SIGPIPE, sigpipe_handler);
    
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
        printf("Missing options. Terminating.\n");
        exit(-1);
    }
    /* Argument Parsing END */


    // First, make connection to server.
    int server_fd = open_clientfd(hostname, port);
    
    if(server_fd < 0) {
        printf("Failed to connect to SERVER %s:%s\n", hostname,port);
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
            unsigned short sum = (~checksum2(buf,CHUNKSIZE)) + (shift<< 8) + opcode + (len & 0xffff) + ((len>>16)&0xffff);
            unsigned short chksum = ~sum;
            uint32_t head1 = (opcode) + (shift << 8) + (chksum<<16);
            uint64_t head_add = ((uint64_t) head1) + (((uint64_t) len)<<32);

            memcpy(packet_sent, (char*) &head_add, 8);
            memcpy(packet_sent+8, buf, CHUNKSIZE);

            rio_writen(server_fd, packet_sent, CHUNKSIZE+8);

            free(packet_sent);
            i=0;
            pack_cnt++;
            rio_writen(STDOUT_FILENO, bufout+0x8, CHUNKSIZE);
            memset(bufout,0,CHUNKSIZE+8);
            memset(buf,0,CHUNKSIZE);
            }
    }
    char* packet_sent = malloc(i+8);
    uint32_t len = htonl(i+8);
    unsigned short sum = (~checksum2(buf,i)) + (shift<<8) + opcode + (len & 0xffff) + ((len>>16)&0xffff);
    unsigned short chksum = ~sum;
    uint32_t head1 = opcode + (shift<<8) + (chksum<<16);
    uint64_t head_add = ((uint64_t) head1) + (((uint64_t) len)<<32);
    memcpy(packet_sent, (char*) &head_add, 8);
    memcpy(packet_sent+8, buf, i);
    rio_writen(server_fd, packet_sent, 8+i);
    free(packet_sent);

/*    
    for(; pack_cnt>0; pack_cnt--){
        if(DEBUG)
            printf("RECV %d bytes from server(%d) >",CHUNKSIZE+8,pack_cnt);
        rio_read(&rio, bufout, CHUNKSIZE+8);
//        printf("%s",bufout+8);
        rio_writen(STDOUT_FILENO, bufout+8, CHUNKSIZE);
        if(DEBUG)
            printf("\n");
        memset(bufout,0,CHUNKSIZE+8);
    }
*/
    rio_read(&rio,bufout,i+8);
    if(DEBUG)
        printf("RECV %d bytes from server >%s\n",8+i,bufout+8);

    rio_writen(STDOUT_FILENO,bufout+8,i);
//    printf("%s",bufout+8);

    // build packet and send()

    
    // recv packets by read()
    // pack_cnt(MAXSIZE+8 bytes) + 1(i+8) times
    // write on FD=0(STDOUT)

    //
   /* 
        int c = EOF;
        unsigned int i=0;                   // BYTES SENT
        unsigned int size_ = CHUNKSIZE;
 
        char *buf = malloc(CHUNKSIZE);
        while( (c=getchar()) != EOF ) {        
            buf[i++] = (char) c;
            if(i == size_){
                size_ = i + CHUNKSIZE;
                buf = realloc(buf, CHUNKSIZE);
            }
        }
        buf[i] = '\0';

        if (i==0) return 1;

        // build packet here (TODO : getopt())

        // buffer to hold server reply
        char buf2[CHUNKSIZE];

        uint32_t len = htonl(8+i);
        unsigned short chksum = ~sum;


        char *packet_sent = malloc(size_+8);
        
        memcpy(packet_sent, (char*) &head_add, 8);
        memcpy(packet_sent+8, buf, i);

      
        if(DEBUG){
        printf("buf = %s\n", buf);
        printf("sum(buf) = 0x%x, sum(op,n) = 0x%x, sum(len) = 0x%x\n",~checksum2(buf,i),( (opcode<<8)+shift), (len & 0xffff) + ((len>>16) & 0xffff));        
        printf("CHECKSUM:0x%x\tSTRING_LEN=%d\n",chksum,i);
        }

        rio_writen(server_fd, packet_sent,i+8);

        memset(buf, 0, size_);
        free(buf);
        free(packet_sent);

        // From server
        ssize_t readbytes;
        if((readbytes = rio_read(&rio, buf2, sizeof(buf2))) <0) {
            printf("Client had received %d Bytes. Terminating.\n", (int) readbytes);
            return -1;
        }


        // Not checking HEADER values(TBI)                
        printf("%s",buf2+8);
        memset(buf2,0,sizeof(buf2));

    */

    return 0;
}


