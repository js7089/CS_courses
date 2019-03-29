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

#define ADDR(x) (ntohl (*(uint32_t *) (x)))
#define PORT(x) (ntohs (*(uint16_t *) (x)))


using namespace std;
void packet_dump(E::Packet* packet){
  uint8_t packet_parsed[54];
  packet->readData(0, packet_parsed, 54);
  hexdump(packet_parsed, 54);
}

void hexdump(void* obj, size_t size){
  for(int i=0; i<size; i++){
    if( *(uint8_t*) ((uint8_t*)obj+i) < 16)
      cout << "0";

    cout <<  hex << (int) *((uint8_t*)obj+i) ;
    cout << " ";
    
    if(i % 16 == 7)
      cout << "\t";
    if(i % 16 == 15)
      cout << endl;
  }
  cout << endl;
}

void node_dump(node nd){
  cout << "[FD = " << nd.sockfd << "] " << endl;
  cout << "(src) " << nd.srcip << ":" << nd.srcport << "<" << nd.seq << ">" << "\t(dest) " << nd.destip << ":" << nd.destport << "<" << nd.ack << ">" << endl;
  cout << "bound/uuid/backlog/used = " << nd.bound << ", " << nd.uuid << ", " << nd.backlog << ", " << nd.used << endl;
}
/* <NODE>
 * sockfd / status / srcip srcport seq / destip destport ack / bound
 */
void node_init(node& nd){
  nd.backlog=0;
  nd.used=0;
  nd.bound=0;
}
void set_backlog(node& nd, int backlog){
  nd.backlog = backlog;
}
void set_addrlen(node& nd, socklen_t addrlen_){
  nd.addrlen = addrlen_;
}
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
void setuuid(node& nd, int uuid_){
    nd.uuid = uuid_;
}
int allocate_port(list<node>& sl){
  int newport;

  generate:
  newport = rand() % (65536-1024) + 1024;

  list<node>::iterator np;
  for(np = sl.begin(); np != sl.end(); ++np){
    if(np->srcport == newport)
      goto generate;
  }
  return newport;
}
void setseq(node& nd, int seq_){
  nd.seq = seq_;
}
void setack(node& nd, int ack_){
  nd.ack = ack_;
}
void incr_seq(node& nd){
  nd.seq++;
}
void incr_ack(node& nd){
  nd.ack++;
}
void set_used(node& nd, int used_){
  nd.used = used_;
}


// List traversal for backlog
// how many 'sockfd's in backlog list?
int num_of_nodes(list<node>& backlog_list, int sockfd_){
  list<node>::iterator np;
  int cnt=0;
  for(np=backlog_list.begin(); np!=backlog_list.end(); ++np){
    if(np->sockfd == sockfd_)
      cnt++;
  }
  return cnt;
}

int in_list(list<node>& backlog_list, int sockfd_, uint32_t addr_, uint16_t port_){

  list<node>::iterator np;
  for(np = backlog_list.begin(); np!=backlog_list.end(); ++np){
    if( np->sockfd == sockfd_ && np->destip == addr_ && np->destport == port_)
      return 1;
  }
  return 0;
}

list<node>::iterator getnodebysockfd(list<node>& sl, int sockfd_){
  list<node>::iterator np;
  for(np = sl.begin(); np != sl.end(); ++np){
    if(np->sockfd == sockfd_)
      return np;
  }
  return sl.end(); 
}



list<node> socklist;
list<node> backlog_list;
list<queue_elem_> accept_queue;

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
  backlog_list.clear();
  socklist.clear();
  accept_queue.clear();
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
        node_init(newnode);
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
    {
      // Extract basic info here
      int targetfd = param.param1_int;
      struct sockaddr* sa = static_cast<struct sockaddr*>(param.param2_ptr);
      struct sockaddr_in* sain = (struct sockaddr_in*) sa;

      socklen_t addrlen = (socklen_t) param.param3_int;

      // Implicit bind      
      uint32_t addr = ntohl(sain->sin_addr.s_addr);
      int port = this->getHost()->getRoutingTable((uint8_t*)&addr);
      uint8_t* ip_buffer = (uint8_t*) malloc(sizeof(int));
      memset(ip_buffer, 0, 4);
    
      this->getHost()->getIPAddr(ip_buffer,port);

      list<node>::iterator node_;
      if( (node_=getnodebysockfd(socklist, targetfd)) == socklist.end() )
        returnSystemCall(syscallUUID, -1);  // socket not found!

      int newport = allocate_port(socklist);
      set_srcaddr(*node_, ntohl(*(uint32_t*)ip_buffer), newport);
      set_destaddr(*node_, addr, ntohs(sain->sin_port));
      setbound(*node_, 1);
      setuuid(*node_, (int) syscallUUID);
      set_addrlen(*node_, addrlen); 

      free(ip_buffer);
    
      // SEND SYN packet to server
//      Packet* syn = allocatePacket(this, 54);

      int seq = rand();
      setseq(*node_, seq);
      setack(*node_, 0);

      // (PACKET SENT HERE)
      Packet* syn = allocatePacket(54);
      uint32_t src_ip = htonl(node_->srcip);
      uint32_t dest_ip = htonl(node_->destip);
      uint16_t src_port = htons(node_->srcport);
      uint16_t dest_port = htons(node_->destport);
      
      uint8_t tcp_seg[54];
      memset(tcp_seg, 0, 54);

      int seq_ = htonl(node_->seq); 
      

      uint16_t hdr_flag = htons((5<<12) + 0x2);  // SYN of hlen=5
      uint16_t wsize = (uint16_t) htons(51200);

      syn->writeData(14+12, (uint8_t*)&src_ip, 4);
      syn->writeData(14+16, (uint8_t*)&dest_ip, 4);
      syn->writeData(14+20, (uint8_t*)&src_port, 2);
      syn->writeData(14+22, (uint8_t*)&dest_port, 2);
      syn->writeData(14+24, (uint8_t*)&seq_, 4);      
      syn->writeData(14+32, (uint8_t*)&hdr_flag, 2);
      syn->writeData(14+34, (uint8_t*)&wsize, 2);

      memcpy(tcp_seg, (uint8_t*)&src_ip, 4);
      memcpy(&tcp_seg[4], (uint8_t*)&dest_ip, 4);
      memcpy(&tcp_seg[8], (uint8_t*)&src_port, 2);
      memcpy(&tcp_seg[10], (uint8_t*)&dest_port, 2);
      memcpy(&tcp_seg[12], (uint8_t*)&seq_, 4);
      memcpy(&tcp_seg[20], (uint8_t*)&hdr_flag, 2);
      memcpy(&tcp_seg[22], (uint8_t*)&wsize, 2);


      uint16_t chksum = htons( 0xffff - NetworkUtil::tcp_sum(src_ip, dest_ip, &tcp_seg[8], 20) );
      syn->writeData(14+36, (uint8_t*)&chksum, 2);


      this->sendPacket("IPv4",syn);

      // Change state and BLOCKs
      set_status(*node_, SYN_SENT);

		break;
    }
	case LISTEN:
  {
    int sockfd = param.param1_int;
    int backlog = param.param2_int;

    list<node>::iterator np;
    if( (np=getnodebysockfd(socklist,sockfd)) == socklist.end() )
      returnSystemCall(syscallUUID, -1);

    set_status(*np, LISTENING);
    set_backlog(*np, backlog);

    returnSystemCall(syscallUUID, 0);
		break;
  }
	case ACCEPT:
  {
    // Argument parsing
    int listenfd = param.param1_int;
    struct sockaddr* sa = static_cast<struct sockaddr*>(param.param2_ptr);
    socklen_t* slen = static_cast<socklen_t*>(param.param3_ptr);

    list<node>::iterator np;

    for(np = socklist.begin(); np!=socklist.end(); ++np){

      // Connection established but not yet returned
      if(np->sockfd==listenfd && np->status==ESTAB && (np->used != 1)){
        int new_fd;
        if((new_fd=createFileDescriptor(pid)) == -1){
          returnSystemCall(np->uuid, -1);
        } else {
          set_sockfd(*np, new_fd);
        }       
        set_used(*np, 1);

        // put connection info to sa and slen
        struct sockaddr_in sain;
        memset(&sain, 0, sizeof(sain));
        sain.sin_family = AF_INET;
        sain.sin_addr.s_addr = htonl(np->destip);
        sain.sin_port = htons(np->destport);        
        memcpy(sa, &sain, sizeof(sain));

        socklen_t slen_data = np->addrlen;
        memcpy(slen, &slen_data, sizeof(socklen_t));
//        *slen = slen_data;

        returnSystemCall(syscallUUID, new_fd);
        break;
      }
    }
    if(np == socklist.end()){
      queue_elem_ waitnode;
      memset(&waitnode, 0, sizeof(queue_elem_));
      waitnode.uuid = syscallUUID;
      waitnode.pid = pid;
      waitnode.sockfd = listenfd;
      waitnode.sa = sa;
      waitnode.slen = slen;
      
      accept_queue.push_back(waitnode);      
    }
        
		//this->syscall_accept(syscallUUID, pid, param.param1_int,
		//		static_cast<struct sockaddr*>(param.param2_ptr),
		//		static_cast<socklen_t*>(param.param3_ptr));
		break;
  }
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
        if(np->srcip==ip_addr || !(np->srcip))
          addr_overlap = 1;
        if(np->srcport==port_) 
          port_overlap = 1;
        if(port_overlap && addr_overlap)
          returnSystemCall(syscallUUID, -1);
          
      }
      for(np=socklist.begin(); np!=socklist.end(); ++np){
        if(np->sockfd == targetfd) {
          if(np->bound)
            returnSystemCall(syscallUUID,-1);
  
          set_sockfd(*np, targetfd);
          set_srcaddr(*np, ip_addr, port_);
          setbound(*np, 1);
          set_addrlen(*np, socklen_);
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
          memset(&sain, 0, (size_t) *socklen);
          sain.sin_family = AF_INET;
          sain.sin_port = htons(np->srcport);
          sain.sin_addr.s_addr = htonl(np->srcip);

          memcpy(sa, &sain, (size_t) *socklen);
          returnSystemCall(syscallUUID,0);
          return;
        }
      }
      returnSystemCall(syscallUUID,-1);
  		break;
    }
	case GETPEERNAME:
  {
    int targetfd = param.param1_int;
    struct sockaddr* sa = static_cast<struct sockaddr*>(param.param2_ptr);
    socklen_t* slen = static_cast<socklen_t*>(param.param3_ptr);

    list<node>::iterator np = getnodebysockfd(socklist, targetfd);
    if(np == socklist.end()){
      returnSystemCall(syscallUUID, -1);
      return;
    }
    struct sockaddr_in sain;
    memset(&sain, 0, sizeof(struct sockaddr_in));
    sain.sin_family = AF_INET;
    sain.sin_port = htons(np->destport);
    sain.sin_addr.s_addr = htonl(np->destip);
    memcpy(sa, &sain, (size_t) sizeof(sain));
    memcpy(slen, &(np->addrlen), sizeof(socklen_t));
    
    returnSystemCall(syscallUUID, 0);

		break;
  }
	default:
		assert(0);
	}
}

void TCPAssignment::packetArrived(std::string fromModule, Packet* packet)
{
//  cout << "packet arrives!" << endl;
  uint8_t src_ip[4];
  uint8_t dest_ip[4];
  uint8_t src_port[2];
  uint8_t dest_port[2];
  uint8_t seq[4];   // remote's seq
  uint8_t ack[4];   // remote's ack == my seq+1
  uint8_t hdr_flag_[2]; // size(Bytes) + flags in NW-byte order

  packet->readData(14+12, src_ip, 4);
  packet->readData(14+16, dest_ip, 4);
  packet->readData(32+2, src_port, 2);
  packet->readData(32+4, dest_port, 2);
  packet->readData(32+6, seq, 4);
  packet->readData(32+10, ack, 4);
  packet->readData(32+14, hdr_flag_, 2);

  uint16_t src_port_ = ntohs(*(uint16_t*)src_port);
  uint16_t dest_port_ = ntohs(*(uint16_t*)dest_port);
  uint32_t seq_ = ntohl(*(uint32_t*)seq);
  uint32_t ack_ = ntohl(*(uint32_t*)ack);
  

  list<node>::iterator np;

  //
  uint16_t flags = 0xff & ntohs(*(uint16_t *)hdr_flag_);
  
  for(np = socklist.begin(); np != socklist.end(); ++np){
    if(np->srcport == PORT(dest_port))
      break;
  }
  int sockfd = -1;
  if(np != socklist.end())
    sockfd = np->sockfd;

  if(flags == 0x02){  // CASE SYN
    size_t limit = np->backlog;

    if(!in_list(backlog_list, sockfd, ADDR(src_ip), PORT(src_port))){
      node newnode; // = (node *) malloc(sizeof(node));
      memcpy(&newnode, &*np, sizeof(node));
      newnode.destip = ADDR(src_ip);
      newnode.destport = PORT(src_port);
      newnode.seq = rand();
      newnode.ack = ntohl((*(uint32_t*)seq))+1;
      newnode.status = SYN_RCVD;

      if(num_of_nodes(backlog_list, sockfd) < limit){   // within limit
        backlog_list.push_front(newnode);

        Packet* synack = this->clonePacket(packet);
        uint8_t tcp_seg[20];
        memset(tcp_seg, 0, 20);

        memcpy(&tcp_seg[0], dest_port, 2);
        memcpy(&tcp_seg[2], src_port, 2);

        uint32_t seq_back = htonl(6974);
        memcpy(&tcp_seg[4], &seq_back, 4);
        uint32_t ack_back = htonl(newnode.ack);
        memcpy(&tcp_seg[8], &ack_back, 4);

        uint16_t hdr_flags = htons((5<<12) + 0x12);
        memcpy(&tcp_seg[12], &hdr_flags, 2);

        uint16_t wsize = htons(51200);
        memcpy(&tcp_seg[14], &wsize, 2);

        uint16_t csum = htons(0xffff - NetworkUtil::tcp_sum(*(uint32_t*)src_ip, *(uint32_t*)dest_ip, tcp_seg, 20));
        memcpy(&tcp_seg[16], &csum, 2);

        synack->writeData(34, tcp_seg, 20);
        synack->writeData(26, dest_ip, 4);
        synack->writeData(30, src_ip, 4);

        this->sendPacket("IPv4", synack);
      }
    }


  }
  else if(flags == 0x10) { // CASE ACK simply POPs out the element for srcip:srcport

    for(np=backlog_list.begin(); np!=backlog_list.end(); ++np){
      if(np->sockfd==sockfd && np->destip==ADDR(src_ip) && np->destport==PORT(src_port) && np->status==SYN_RCVD){
        node newnode;
        memcpy(&newnode, &*np, sizeof(node));
        newnode.status = ESTAB;

        backlog_list.erase(np);

        if(!accept_queue.size()){
          socklist.push_front(newnode);
        } else {
          list<queue_elem_>::iterator qp = accept_queue.begin();
          int new_fd = createFileDescriptor(qp->pid);
          if(new_fd == -1){
            accept_queue.pop_front();
            returnSystemCall(qp->uuid, -1);
          }
          newnode.sockfd = new_fd;
          newnode.used = 1;

//          set_sockfd(*np, new_fd);
//          set_used(*np, 1);
          int saved_uuid = qp->uuid;

          struct sockaddr_in sain;
          memset(&sain, 0, sizeof(sain));
          sain.sin_family = AF_INET;
          sain.sin_port = htons(newnode.srcport);
          sain.sin_addr.s_addr = htonl(newnode.srcip);

          memcpy(qp->sa, &sain, (size_t) sizeof(sain));
          memcpy(qp->slen, &np->addrlen, sizeof(socklen_t));

          accept_queue.pop_front();
          socklist.push_front(newnode);
          returnSystemCall(saved_uuid, new_fd);
        }
        break;
      }
        
    }
  }
  //

  // send SYN to server
  for(np = socklist.begin(); np != socklist.end(); ++np){
    if(np->srcport==dest_port_ && np->status==SYN_SENT){
      // change the socket state to ESTAB
      np->status = ESTAB;

      // set seq, ack, and send ACK back
      setseq(*np,ack_);
      setack(*np,seq_+1);
      Packet* estab_syn = this->clonePacket(packet);
      estab_syn->writeData(14+12, dest_ip, 4);
      estab_syn->writeData(14+16, src_ip, 4);
      estab_syn->writeData(32+2, dest_port,2);
      estab_syn->writeData(32+4, src_port, 2);

      uint16_t hdr_flag = htons( (5<<12) + 0x10 );
      estab_syn->writeData(46, (uint8_t*)&hdr_flag, 2);

      uint32_t seq_sent = htonl(np->seq);
      uint32_t ack_sent = htonl(np->ack);
      estab_syn->writeData(32+6, (uint8_t*)&seq_sent, 4);
      estab_syn->writeData(32+10, (uint8_t*)&ack_sent, 4);

      uint8_t tcp_seg[20];
      estab_syn->readData(34, tcp_seg, 20);
      memset(&tcp_seg[16], 0, 2); // chksum to zero

      uint16_t csum = htons(0xffff - NetworkUtil::tcp_sum(*(uint32_t*)src_ip,*(uint32_t*)dest_ip, tcp_seg, 20)) ;
      estab_syn->writeData(50, (uint8_t*)&csum, 2);
      this->sendPacket("IPv4", estab_syn);

      // then return
      returnSystemCall(np->uuid, 0);
    }
  }
  

  
}

void TCPAssignment::timerCallback(void* payload)
{

}


}
