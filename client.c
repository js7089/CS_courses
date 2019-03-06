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
#define CHUNKSIZE 65536 
#define DEBUG 0 


#define HEADER(op,n,chksum,len) ((((htons(op)<<8) + htons(n))<<48) + (htons(chksum)<<32) + htonl(len))

void sigpipe_handler(int sig){
    printf("[CHECKSUM] Validation failed.\n");
}


int main(int argc, char** argv){

    signal(SIGPIPE, sigpipe_handler);

    // First, make connection to server.
    int server_fd = open_clientfd(SERVER, PORT);
    
    if(server_fd < 0) {
        printf("Failed to connect to SERVER %s:%s\n", SERVER, PORT);
        return -1;
    }

    rio_t rio;
    
    rio_readinitb(&rio, server_fd);

    
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
        unsigned char opcode = 0;
        unsigned char shift = 0;

        // buffer to hold server reply
        char buf2[CHUNKSIZE];

        uint32_t len = htonl(8+i);
        unsigned short sum = (~checksum2(buf,i)) + (opcode << 8) + shift + (len & 0xffff) + ((len>>16)&0xffff);
        unsigned short chksum = ~sum;

        uint32_t head1 = (opcode) + (shift << 8) + (chksum<<16);
        uint64_t head_add = ((uint64_t) head1) + (((uint64_t) len)<<32);

        char *packet_sent = malloc(size_+8);
        
        memcpy(packet_sent, (char*) &head_add, 8);
        memcpy(packet_sent+8, buf, i);

        /** DEBUGGING PURPOSE **/

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
        printf("Reply from server: [%s]\n",buf2+8);
        memset(buf2,0,sizeof(buf2));

    

    return 0;
}


