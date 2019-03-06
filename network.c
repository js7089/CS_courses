#include "network.h"

int open_clientfd(char* host, char* port){
    int clientfd, rc;
    struct addrinfo hints, *listp, *p;

    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV | AI_ADDRCONFIG;

    if((rc = getaddrinfo(host, port, &hints, &listp)) != 0) 
        return -2;

    for(p=listp; p; p=p->ai_next){
        if((clientfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))<0)
            continue;
    
        if(connect(clientfd, p->ai_addr, p->ai_addrlen) != -1)
            break;
        if(close(clientfd)<0) 
            return -1;
        
    }
    freeaddrinfo(listp);
    if(!p)
        return -1;
    else
        return clientfd;
}

int open_listenfd(char* port){
    struct addrinfo hints, *listp, *p;
    int listenfd, rc, optval=1;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG | AI_NUMERICSERV;
    if((rc = getaddrinfo(NULL, port, &hints, &listp)) != 0)
        return -2;
    
    for(p=listp; p; p=p->ai_next){
        if((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) <0)
            continue;

        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));

        if(bind(listenfd, p->ai_addr, p->ai_addrlen) == 0)
            break;
        if(close(listenfd) <0)
            return -1;
    }

freeaddrinfo(listp);
if(!p) return -1;

if(listen(listenfd, 1024) <0){
    close(listenfd);
    return -1;
}

return listenfd;
}

unsigned short checksum2(const char *buf, unsigned size)
{
    unsigned long long sum = 0;
    const unsigned long long *b = (unsigned long long *) buf;

    unsigned t1, t2;
    unsigned short t3, t4;

    /* Main loop - 8 bytes at a time */
    while (size >= sizeof(unsigned long long))
    {
        unsigned long long s = *b++;
        sum += s;
        if (sum < s) sum++;
        size -= 8;
    }

    /* Handle tail less than 8-bytes long */
    buf = (const char *) b;
    if (size & 4)
    {
        unsigned s = *(unsigned *)buf;
        sum += s;
        if (sum < s) sum++;
        buf += 4;
    }

    if (size & 2)
    {
        unsigned short s = *(unsigned short *) buf;
        sum += s;
        if (sum < s) sum++;
        buf += 2;
    }

    if (size)
    {
        unsigned char s = *(unsigned char *) buf;
        sum += s;
        if (sum < s) sum++;
    }

    /* Fold down to 16 bits */
    t1 = sum;
    t2 = sum >> 32;
    t1 += t2;
    if (t1 < t2) t1++;
    t3 = t1;
    t4 = t1 >> 16;
    t3 += t4;
    if (t3 < t4) t3++;

    return ~t3;
}

/* RIO FUNCTIONS */
void rio_readinitb(rio_t* rp, int fd){
    rp->rio_fd = fd;
    rp->rio_cnt = 0;
    rp->rio_bufptr = rp->rio_buf;    

}

ssize_t rio_read(rio_t* rp, void* usrbuf, size_t n){
    int cnt;
    while(rp->rio_cnt <= 0) {
        rp->rio_cnt = read(rp->rio_fd, rp->rio_buf, sizeof(rp->rio_buf));
        if(rp->rio_cnt < 0)
            return -1;
        else if(rp->rio_cnt==0)
            return 0;
        else
            rp->rio_bufptr = rp->rio_buf;
    }

    cnt = n;
    if(rp->rio_cnt < n)
        cnt = rp->rio_cnt;
    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;
    rp->rio_cnt -= cnt;
    return cnt;
}

ssize_t rio_writen(int fd, void* usrbuf, size_t n){
    size_t nleft = n;
    ssize_t nwritten;
    char* bufp = usrbuf;

    while(nleft>0) {
        if ((nwritten = write(fd,bufp,nleft)) < 0)
            return -1;
        
        nleft -= nwritten;
        bufp += nwritten;
    }

    return n;

}


