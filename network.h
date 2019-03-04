#ifndef __NETWORK_H__
#define __NETWORK_H__

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

typedef struct sockaddr SA;

unsigned short checksum2(const char *buf, unsigned size);

int open_clientfd(char* host, char* port);
int open_listenfd(char* port);


/* CLIENT */
int Connect(int sockfd, struct sockaddr* server, int addrlen);

/* SERVER */
int Socket(int domain, int type, int protocol);
int Bind(int sockfd, struct sockaddr *my_addr, int addrlen);
int Listen(int s, int backlog);
int Accept(int s, struct sockaddr* addr, socklen_t* addrlen);

#endif
