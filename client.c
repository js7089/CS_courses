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

#define HEADER(op,n,chksum,len) ((((htons(op)<<8) + htons(n))<<48) + (htons(chksum)<<32) + htonl(len))
/*   8bit 8bit   16bit         32bit
 *  | op |  n | chksum |        len        |
 */

void sigpipe_handler(int sig){
    printf("program received SIGPIPE.\n");
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

    while(1){
        int c = EOF;
        unsigned int i=0;                   // BYTES SENT
        unsigned int size_ = CHUNKSIZE;

        char *buf = malloc(CHUNKSIZE); 
        while( (c=getchar()) != '\n' && c != EOF ) {        
            buf[i++] = (char) c;
            if(i == size_){
                size_ = i + CHUNKSIZE;
                buf = realloc(buf, CHUNKSIZE);
            }
        }
        if (i==0) return 1;

        // build packet here (TODO : getopt())
        unsigned char opcode = 0;
        unsigned char shift = 0;

        // buffer to hold server reply
        char buf2[CHUNKSIZE];
        
        unsigned short chksum = ~(~checksum2(buf,i) + ~checksum2((char*)&i,4) + (opcode + shift));
        
        uint32_t head1 = htonl( (opcode << 24) + (shift << 16) + chksum );
        uint32_t len = htonl(i);

        rio_writen(server_fd, (char*) &head1, 4);
        rio_writen(server_fd, (char*) &len, 4);
        rio_writen(server_fd, buf, i);

        free(buf);

        // From server
        rio_read(&rio, buf2, i+8);
        printf("Reply from server: [%s]\n",buf2);
    }

    return 0;
}


