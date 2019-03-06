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
    printf("SERVER_FD = %d\n",server_fd);
    
    unsigned char n=2;  // shift N
    rio_t rio;
    
    rio_readinitb(&rio, server_fd);

    while(1){
        int c = EOF;
        unsigned int i=0;
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

        // build packet here
        unsigned char opcode=0;
        

        /*
         *  <- op -><- n  -><- chksum ->
         *  <---       length       --->
         *  <- payload -->
        */

        char *buf2 = malloc(CHUNKSIZE);

        printf("OP\tN\tCHKSUM\n");
        printf("0x%x\t\t0x%x\n",htons((opcode<<4)+i),htons(checksum2(buf,i)));

        unsigned short chksum = ~(~checksum2(buf,i) + ~checksum2((char*)&i,4));


        unsigned long header = 0 + (htons(chksum)<<16) + htons(i);

        printf("HEADER = 0x%lx\n",header);
            //(i<<16) + ((checksum2(buf,i)<<8)) + (n<<4) + opcode;
        printf("PAYLOAD = %d",i);

        int64_t test = HEADER(0,0,0xffff,0);        
        rio_writen(server_fd,(char*) &test, 8);

        //rio_writen(server_fd,(char*) &header, 8);
        //rio_writen(server_fd,buf,i);

        free(buf);

        rio_read(&rio, buf2, sizeof(buf2));
        printf("[%s]\n",buf2);
    }

    return 0;
}


