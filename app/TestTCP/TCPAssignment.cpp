/*
 * E_TCPAssignment.cpp
 *
 *  Created on: 2014. 11. 20.
 *      Author: Keunhong Lee
 */


#include <E/E_Common.hpp>
#include <E/Networking/E_Host.hpp>
#include <E/Networking/E_Networking.hpp>
#include <cerrno>
#include <E/Networking/E_Packet.hpp>
#include <E/Networking/E_NetworkUtil.hpp>
#include "TCPAssignment.hpp"

using namespace std;


typedef struct DescriptorInfo{
    int sockfd;
    uint32_t ipaddr;
    unsigned short port;
    int bound;
} node;

void setval(node& nd, int sockfd_, int ipaddr_, unsigned short port_, int bound_){
  nd.sockfd = sockfd_;
  nd.ipaddr = ipaddr_;
  nd.port = port_;
  nd.bound = bound_;
}


list<node> dl;


void list_view(){
    list<node>::iterator np;

    if(dl.empty()){
      cout << "EMPTY LIST" << endl;
      return;
    }
    for(np=dl.begin(); np!=dl.end(); ++np){
      cout << "[" << np->sockfd << "] " ;
      cout << (((np->ipaddr) >>24)&0xff) << "." << (((np->ipaddr) >>16)&0xff) << "." ;
      cout << (((np->ipaddr)>>8)&0xff) << "." << ((np->ipaddr)&0xff) << ":" ;
      cout << np->port << " (" << ((np->bound)? ("using"):("unused")) << ")" << endl;
    }
}


namespace E
{

TCPAssignment::TCPAssignment(Host* host) : HostModule("TCP", host),
		NetworkModule(this->getHostModuleName(), host->getNetworkSystem()),
		SystemCallInterface(AF_INET, IPPROTO_TCP, host),
		NetworkLog(host->getNetworkSystem()),
		TimerModule(host->getSystem())
{

}

TCPAssignment::~TCPAssignment()
{


}

void TCPAssignment::initialize()
{
  dl.clear();
}

void TCPAssignment::finalize()
{

}

void TCPAssignment::systemCallback(UUID syscallUUID, int pid, const SystemCallParameter& param)
{
	switch(param.syscallNumber)
	{
	case SOCKET:
  {
    int new_fd = createFileDescriptor(pid);

    if(new_fd == -1)
      returnSystemCall(syscallUUID, -1);
    else {
      node newnode ;
      setval(newnode, new_fd, 0, 0, 0);
      dl.push_front(newnode);
      returnSystemCall(syscallUUID, new_fd);
    } 

		break;
  }

	case CLOSE:
  {
    int target_fd = param.param1_int;
    int success = -1;

    list<node>::iterator np;

    for(np=dl.begin(); np!=dl.end(); ++np){
      if( np->sockfd == target_fd){
        dl.erase(np);
        removeFileDescriptor(pid,np->sockfd);
        success = 0;
        break;
      } 
    }
    
    returnSystemCall(syscallUUID, success);
    break;
  }
	case READ:
		//this->syscall_read(syscallUUID, pid, param.param1_int, param.param2_ptr, param.param3_int);
		break;
	case WRITE:
		//this->syscall_write(syscallUUID, pid, param.param1_int, param.param2_ptr, param.param3_int);
		break;
	case CONNECT:
		//this->syscall_connect(syscallUUID, pid, param.param1_int,
		//		static_cast<struct sockaddr*>(param.param2_ptr), (socklen_t)param.param3_int);
		break;
	case LISTEN:
		//this->syscall_listen(syscallUUID, pid, param.param1_int, param.param2_int);
		break;
	case ACCEPT:
		//this->syscall_accept(syscallUUID, pid, param.param1_int,
		//		static_cast<struct sockaddr*>(param.param2_ptr),
		//		static_cast<socklen_t*>(param.param3_ptr));
		break;
	case BIND:
    {
    int targetfd = param.param1_int;
    struct sockaddr_in* my_addr = (struct sockaddr_in*) static_cast<struct sockaddr*>(param.param2_ptr);
    uint16_t port_ = ntohs(my_addr->sin_port);
    uint32_t ip_addr = ntohl(my_addr->sin_addr.s_addr);

    socklen_t socklen_ = param.param3_int;

    list<node>::iterator np;

    for(np=dl.begin(); np!=dl.end(); ++np){
      int port_overlap = 0;
      int addr_overlap = 0;

      if(!np->bound)
        continue;   // the node isn't bound to anywhere
      if(np->ipaddr==ip_addr || !(np->ipaddr))
        addr_overlap = 1;
      if(np->port==port_) 
        port_overlap = 1;
      if(port_overlap && addr_overlap)
        returnSystemCall(syscallUUID, -1);
    }
    for(np=dl.begin(); np!=dl.end(); ++np){
      if(np->sockfd == targetfd) {
        if(np->bound)
          returnSystemCall(syscallUUID,-1);

        setval(*np, targetfd, ip_addr, port_, 1);
        returnSystemCall(syscallUUID,0);
      }
    }
    break;
/*
		//this->syscall_bind(syscallUUID, pid, param.param1_int,
		//		static_cast<struct sockaddr *>(param.param2_ptr),
		//		(socklen_t) param.param3_int);
		break;
*/
    }
	case GETSOCKNAME:
    {
    list<node>::iterator np;
    int targetfd = param.param1_int;  
    struct sockaddr* sa = static_cast<struct sockaddr*>(param.param2_ptr);
    socklen_t* socklen = static_cast<socklen_t*>(param.param3_ptr);

    for(np=dl.begin(); np!=dl.end(); ++np){
      if(np->sockfd == targetfd){   // socket descriptor found!
        struct sockaddr_in sain;
        memset(&sain, 0, sizeof(struct sockaddr));
        sain.sin_family = AF_INET;
        sain.sin_port = htons(np->port);
        sain.sin_addr.s_addr = htonl(np->ipaddr);
        
        memcpy(sa, &sain, (size_t) *socklen);
        returnSystemCall(syscallUUID,0);
      }
    }
    returnSystemCall(syscallUUID,-1);

		//this->syscall_getsockname(syscallUUID, pid, param.param1_int,
		//		static_cast<struct sockaddr *>(param.param2_ptr),
		//		static_cast<socklen_t*>(param.param3_ptr));
		break;
    }
	case GETPEERNAME:
		//this->syscall_getpeername(syscallUUID, pid, param.param1_int,
		//		static_cast<struct sockaddr *>(param.param2_ptr),
		//		static_cast<socklen_t*>(param.param3_ptr));
		break;
	default:
		assert(0);
	}
}

void TCPAssignment::packetArrived(std::string fromModule, Packet* packet)
{

}

void TCPAssignment::timerCallback(void* payload)
{

}


}
