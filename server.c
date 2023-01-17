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
#include "cesar.h"

#define CHUNKSIZE 65536 
#define DEBUG 0 
#define MAXLEN 200
#define BATCH 1500

void sigchld_handler(int sig){
    int status;
    while(waitpid(-1,&status,WNOHANG)>0){
        ;
    }
}

int main(int argc, char** argv){

    // Server shouldn't "Terminate" on broken pipe
    // Reap zombies
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, sigchld_handler);

    /* Argument Parsing */
    int tag;
    char port[MAXLEN];
    memset(port, 0, MAXLEN);

    int flag_p=0;

    while( (tag = getopt(argc, argv, "p:")) != -1) {
        switch(tag){
            case 'p':
                flag_p=1;
                memcpy(port, optarg, strlen(optarg));
                break;
        }
    }
    if( !flag_p ){
        exit(-1);
    }  

    // listen for client at specified port
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    listenfd = open_listenfd(port);

    // init
    rio_t rio;
        
    while(1){
        clientlen = sizeof(struct sockaddr_storage);
        connfd = accept(listenfd, (SA *)&clientaddr, &clientlen);

        if(fork() == 0){
            close(listenfd);
            // handle request here

            rio_readinitb(&rio, connfd);

            char strbuf[CHUNKSIZE+8];
            char strout[CHUNKSIZE+8];
            ssize_t rbytes = 0;

            while( (rbytes = rio_read(&rio, strbuf, BATCH)) > 0){
                char* packet_sent = malloc(rbytes);
                uint32_t len = htonl(rbytes);

                unsigned char opcode = strbuf[0];
                unsigned char shift = strbuf[1];

                if( checksum2(strbuf,rbytes) ){
                    close(connfd);
                    exit(-1);
                }


                if((opcode&0xfe)){  // invalid input
                    close(connfd);
                    exit(-1);
                }
                uint32_t head1 = (opcode) + (shift<<8);
                head1 &= 0xffff;
                uint64_t head_add = ((uint64_t) head1) + (((uint64_t) len) <<32);
                memcpy(packet_sent, (char*) &head_add, 8);
                for(int k=0; k<rbytes-8; k++){
                    strout[k+8] = cesar(strbuf[k+8],(int) opcode & 0x1,(int) shift & 0xff);
                }
                memcpy(packet_sent+8, strout+8, rbytes-8);

                unsigned short chksum = checksum2(packet_sent, rbytes);
                head1 += (chksum<<16);
                head_add = ((uint64_t) head1) + (((uint64_t) len)<<32);
                memcpy(packet_sent, (char*) &head_add, 8);

                rio_writen(connfd, packet_sent, rbytes);
                free(packet_sent);
                memset(strbuf,0,CHUNKSIZE+8);
                memset(strout,0,CHUNKSIZE+8);
                rbytes=0; 
            }
            close(connfd);
            exit(0);
        }
        close(connfd);
    }
    return 0;
}
