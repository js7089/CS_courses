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

struct node {
    int sockfd;
    struct node* prev;
    struct node* next;
} ;

struct node* socklist;

using namespace std;

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
    struct node* head = (struct node*) malloc(sizeof(struct node*));
    struct node* tail = (struct node*) malloc(sizeof(struct node*));

    head->prev = NULL;
    tail->next = NULL;
    head->next = tail;
    tail->prev = head;
    socklist = head;
   
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
                int sockfd = SystemCallInterface::createFileDescriptor(pid);

                if(sockfd != -1){
                    struct node* newnode = (struct node*) malloc(sizeof(struct node));
                    newnode->sockfd = sockfd;
                    newnode->next = socklist->next;
                    newnode->prev = socklist;
                    socklist->next->prev = newnode;
                    socklist->next = newnode;
                }
                SystemCallInterface::returnSystemCall(syscallUUID, sockfd);
                }
		break;
	case CLOSE:
                {
                cout << "close socket = " << paramsockfd << endl;
                struct node* lm = socklist;
                int sockfd = -1;
                for( ; lm->next->sockfd != -2; lm=lm->next){
                    if(lm->sockfd == (int)socket)
                        break;
                }
                if(sockfd==-1){
                    std::cout << "sockfd not found. Stop." << std::endl;
                    SystemCallInterface::returnSystemCall(syscallUUID, sockfd);
                }else{
                    lm->next->prev = lm->prev;
                    lm->prev->next = lm->next;
                    SystemCallInterface::removeFileDescriptor(pid, sockfd);
                    free(lm);
                    SystemCallInterface::returnSystemCall(syscallUUID, 0);              
                }
                 
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
		//this->syscall_bind(syscallUUID, pid, param.param1_int,
		//		static_cast<struct sockaddr *>(param.param2_ptr),
		//		(socklen_t) param.param3_int);
		break;
	case GETSOCKNAME:
		//this->syscall_getsockname(syscallUUID, pid, param.param1_int,
		//		static_cast<struct sockaddr *>(param.param2_ptr),
		//		static_cast<socklen_t*>(param.param3_ptr));
		break;
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
