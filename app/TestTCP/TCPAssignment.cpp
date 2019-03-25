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

/* <NODE>
 * sockfd / status / srcip srcport seq / destip destport ack / bound
 */

void set_sockfd(node& nd, int sockfd_){
    nd.sockfd = sockfd_;
}
void set_status(node& nd, conn_status stat_){
    nd.status = stat_;
}
void set_srcaddr(node& nd, uint32_t srcip_, uint16_t srcport_){
    nd.srcip = srcip_;
    nd.srcport = srcport_;
}
void set_destaddr(node& nd, uint32_t destip_, uint16_t destport_){
    nd.destip = destip_;
    nd.destport = destport_;
}
void setbound(node& nd, int bound_){
    nd.bound = bound_;
}

list<node> socklist;


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
  socklist.clear();
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
        set_sockfd(newnode, new_fd);
        setbound(newnode, 0);
        socklist.push_front(newnode);
        returnSystemCall(syscallUUID, new_fd);
      } 

		  break;
    }

	case CLOSE:
    {
      int target_fd = param.param1_int;
      int success = -1;

      list<node>::iterator np;
  
      for(np=socklist.begin(); np!=socklist.end(); ++np){
        if( np->sockfd == target_fd){
          socklist.erase(np);
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
  
      for(np=socklist.begin(); np!=socklist.end(); ++np){
        int port_overlap = 0;
        int addr_overlap = 0;
  
        if(!np->bound)
          continue;   // the node isn't bound to anywhere
        if(np->destip==ip_addr || !(np->destip))
          addr_overlap = 1;
        if(np->destport==port_) 
          port_overlap = 1;
        if(port_overlap && addr_overlap)
          returnSystemCall(syscallUUID, -1);
          
      }
      for(np=socklist.begin(); np!=socklist.end(); ++np){
        if(np->sockfd == targetfd) {
          if(np->bound)
            returnSystemCall(syscallUUID,-1);
  
          set_sockfd(*np, targetfd);
          set_destaddr(*np, ip_addr, port_);
          setbound(*np, 1);
          returnSystemCall(syscallUUID,0);
        }
      }
      break;
    }
	case GETSOCKNAME:
    {
      list<node>::iterator np;
      int targetfd = param.param1_int;  
      struct sockaddr* sa = static_cast<struct sockaddr*>(param.param2_ptr);
      socklen_t* socklen = static_cast<socklen_t*>(param.param3_ptr);

      for(np=socklist.begin(); np!=socklist.end(); ++np){
        if(np->sockfd == targetfd){   // socket descriptor found!
          struct sockaddr_in sain;
          memset(&sain, 0, sizeof(struct sockaddr));
          sain.sin_family = AF_INET;
          sain.sin_port = htons(np->destport);
          sain.sin_addr.s_addr = htonl(np->destip);
          
          memcpy(sa, &sain, (size_t) *socklen);
          returnSystemCall(syscallUUID,0);
        }
      }
      returnSystemCall(syscallUUID,-1);
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
