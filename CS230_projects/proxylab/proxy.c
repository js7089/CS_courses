#include <stdio.h>
#include <strings.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define PORT "4166"
#define MAXARGS 1024
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

void request(int connfd){
    char buf[MAXLINE], buf2[MAXLINE];
    size_t n, n2;
    rio_t rio, rio2;


    // PART1. parse request from proxy-client
    Rio_readinitb(&rio,connfd);    
    char linefirst[MAXLINE];
    Rio_readlineb(&rio,buf,MAXLINE);
    printf("%s",buf);
    strcpy(linefirst,buf);

    // incoming requests
    do {
	n = Rio_readlineb(&rio, buf, MAXLINE);
	printf("%s",buf);
    }while(strcmp(buf,"\r\n"));

    // parse request string here
    char* tok;
    char host[MAXLINE],suffix[MAXLINE],tmp[MAXLINE], port[5];
    // ex : host=klms.kaist.ac.kr:80, suffix=/index.html
    memset(host,0,MAXLINE);
    memset(suffix,0,MAXLINE);
    memset(tmp,0,MAXLINE);
    memset(port,0,5);
    tok = strtok(linefirst," ");
    tok = strtok(NULL," ");
    sprintf(tmp,"%s",tok);

    if(strncasecmp(tmp, "http://", 7) != 0){
	printf("NOT HTTP request : host(%s)\n",tmp);
	return;
    }
//    printf("%s\n",tmp);
    tok = strtok(tmp,"/");
    tok = strtok(NULL,"/");
    strncpy(host, tok, strlen(tok));
    
    tok = strtok(NULL,"/");
    if(!tok){ strcpy(suffix,"/"); }
    else{
	sprintf(suffix,"");
	while(tok){ 
	    sprintf(suffix,"%s/%s",suffix,tok);
	    tok = strtok(NULL,"/");
	}
    }

    tok = strstr(host,":");

    if(tok){
	memset(tok,0,1);
	strncpy(port,tok+1,strlen(tok+1));
    }else{
	sprintf(port,"80");
    }
    // parsing ends here
    
    // PART2. request to server
    int clientfd = Open_clientfd(host, port);

    // build string
    sprintf(buf2,"GET %s HTTP/1.0\r\n",suffix);
    sprintf(buf2,"%sHOST: %s\r\n",buf2,host);
    sprintf(buf2,"%s%s",buf2,user_agent_hdr);
    sprintf(buf2,"%sConnection: close\r\n",buf2);
    sprintf(buf2,"%sProxy-Connection: close\r\n\r\n",buf2);

    printf("To server : \n%s",buf2);
    Rio_writen(clientfd,buf2,sizeof(buf2));

    Rio_readinitb(&rio2,clientfd);
    char result[109600];
    sprintf(result,"");
     do{
	n = Rio_readlineb(&rio2,buf,MAXLINE);
	sprintf(result,"%s%s",result,buf);
    }while(strcmp(buf,"\r\n"));

    while( (n=Rio_readlineb(&rio2,buf,MAXLINE)) != 0){
	sprintf(result, "%s%s", result,buf);
    }
/*    do{
	n = Rio_readlineb(&rio2,buf,MAXLINE);
	sprintf(result,"%s%s",result,buf);
	printf("%s\r\n",buf);
    }while(strcmp(buf,"\r\n"));
*/
    Close(clientfd);
    printf("From server:\n%s",result);
    Rio_writen(connfd,result,strlen(result));

    return;

}

void sigchld_handler(int sig){
    int status;
    while(waitpid(-1,&status,WNOHANG)>0)
	;
}

int main(int argc, char** argv)
{
    char lis_port[5];
    if(argv[1] == NULL){
	sprintf(lis_port,PORT);
    }else{
	sprintf(lis_port,"%s",argv[1]);
    }

    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
//    char client_hostname[MAXLINE], client_port[MAXLINE];

    Signal(SIGCHLD,sigchld_handler);
    
    listenfd = Open_listenfd(lis_port);
    while(1){
	clientlen = sizeof(struct sockaddr_storage);
	connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
	if(Fork() == 0){
	    // do request
	    Close(listenfd);
	    request(connfd);
	    Close(connfd);
	    exit(0);
	}
	Close(connfd);


    }
    printf("%s", user_agent_hdr);
    return 0;
}
