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
#include <E/E_TimeUtil.hpp>
#include <E/E_TimerModule.hpp>
#include "TCPAssignment.hpp"

#define ADDR(x) (ntohl (*(uint32_t *) (x)))
#define PORT(x) (ntohs (*(uint16_t *) (x)))

#define DEBUG 0 

#define WIN (5<<12)
#define URG 0x20
#define ACK 0x10
#define PSH 0x8
#define RST 0x4
#define SYN 0x2
#define FIN 0x1

using namespace std;

/* Helper function prototypes */
void check_fd(int pid, int fd);
void build_packet(E::Packet* pkt, uint32_t srcip, uint16_t srcport, uint32_t destip, uint16_t destport, uint32_t seq, uint32_t ack, uint16_t hdr_flag, uint16_t winsize);
void build_packet_data(E::Packet* pkt, uint32_t srcip, uint16_t srcport, uint32_t destip, uint16_t destport, uint32_t seq, uint32_t ack, uint16_t hdr_flag, uint16_t winsize, uint8_t* data, size_t datalen);
void state(node nd);
void packet_dump(E::Packet* packet);
void hexdump(void* obj, size_t size);
void set_owner(node& nd, int pid);
void node_dump(node nd);
void node_init(node& nd);
void set_backlog(node& nd, int backlog);
void set_addrlen(node& nd, socklen_t addrlen);
void set_sockfd(node& nd, int sockfd_);
void set_status(node& nd, int sockfd_);
void set_srcaddr(node& nd, uint32_t srcip_, uint16_t srcport_);
void set_destaddr(node& nd, uint32_t destip_, uint16_t destport_);
void setbound(node& nd, int bound);
void setuuid(node& nd, int uuid_);
int allocate_port(list<node>& sl);
void setseq(node& nd, int seq_);
void setack(node& nd, int ack_);
void incr_seq(node& nd);
void set_used(node& nd, int used_);
void traverse(list<node> nd);
int num_of_nodes(list<node>& backlog_list, int sockfd_, int pid);
int in_list(list<node>& backlog_list, int sockfd_, uint32_t addr_, uint16_t port_);
list<node>::iterator getnodebysockfd(list<node>& sl, int sockfd_, int pid);

/* Global variables */
list<node> socklist;
list<node> backlog_list;
list<queue_elem_> accept_queue;

/* TCPAssignment.cpp code start */
namespace E
{

TCPAssignment::TCPAssignment(Host* host) : HostModule("TCP", host),
		NetworkModule(this->getHostModuleName(), host->getNetworkSystem()),
		SystemCallInterface(AF_INET, IPPROTO_TCP, host),
		NetworkLog(host->getNetworkSystem()),
		TimerModule(host->getSystem())
{

}
void TCPAssignment::send_segments(node& np, size_t seg) {
  if(np.send_len == BUFSIZE) return;

  for(size_t cnt = 0; cnt < seg; seg++) {
    size_t payload_len = (BUFSIZE - np.send_len <= MSS)? (BUFSIZE - np.send_len) : MSS;
    uint8_t data[payload_len];
    
    if(np.end_s + payload_len >= BUFSIZE) {
      memcpy(data, &(np.send_buffer[np.end_s]), BUFSIZE - np.end_s);
      memcpy(&data[BUFSIZE-np.end_s], &np.send_buffer, payload_len+np.end_s-BUFSIZE);
    } else {
      memcpy(data, &(np.send_buffer[np.end_s]), payload_len);
    }

    np.end_s += payload_len;
    np.send_len += payload_len;

    if(np.end_s >= BUFSIZE) np.end_s -= BUFSIZE;
    
    /* Build packet here */
    Packet* pkt = allocatePacket(54+payload_len);
    uint8_t ip_buffer[4];
    this->getHost()->getIPAddr(ip_buffer, 0);
    uint32_t myip = (np.srcip)? np.srcip : htonl(*(uint32_t *) ip_buffer);
    
    build_packet_data(pkt, myip, np.srcport, np.destip, np.destport, np.seq, np.ack, WIN | ACK, 51200 - np.recv_len, data, payload_len);
    this->sendPacket("IPv4", pkt);
    np.seq += payload_len;
    if(np.send_len == BUFSIZE) {
      np.sending = false;
      break;
    }
  }
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
  backlog_list.clear();
  socklist.clear();
  accept_queue.clear();
}

void TCPAssignment::systemCallback(UUID syscallUUID, int pid, const SystemCallParameter& param)
{
	switch(param.syscallNumber)
	{
	case SOCKET:
    {
      int new_fd = createFileDescriptor(pid);
      check_fd(pid, new_fd);
      // x01 

      if(new_fd == -1)
        returnSystemCall(syscallUUID, -1);
      else {
        node newnode ;
        //memset(&newnode, 0, sizeof(newnode));
        newnode.owner = pid;
        set_sockfd(newnode, new_fd);
        node_init(newnode);
        setbound(newnode, 0);
        socklist.push_front(newnode);

        if(DEBUG){
          printf("New socket (fd=%d, pid=%d) was created!!\n",newnode.sockfd, newnode.owner);
          node_dump(newnode);        
        }
        returnSystemCall(syscallUUID, newnode.sockfd);
      } 

		  break;
    }

	case CLOSE:
    {
      int target_fd = param.param1_int;
      int success = -1;
      list<node>::iterator np;
      uint8_t* ip_buffer = (uint8_t*) malloc(sizeof(int));
      memset(ip_buffer,0,4);
      this->getHost()->getIPAddr(ip_buffer, 0);
 
      for(np=socklist.begin(); np!=socklist.end(); ++np){
        if( np->sockfd == target_fd && np->owner == pid && np->status == ESTAB){
          if(DEBUG) printf("[CLOSE] Remaining %d bytes to be sent\n", BUFSIZE - np->send_len);
          if(!np->sending || 1){
            send_segments(*np, 100);
         }

          np->bound = 0;
          /* Build FIN packet here */
          Packet* pkt = this->allocatePacket(54);
          uint32_t myip = (np->srcip)? np->srcip : htonl(*(uint32_t *) ip_buffer);

          build_packet(pkt, myip, np->srcport, np->destip, np->destport, np->seq, 0, (WIN | FIN), 51200);
          this->sendPacket("IPv4", pkt);
          np->status = FIN_WAIT_1;
          np->uuid = syscallUUID;
          if(DEBUG) node_dump(*np);
          break;
          }
        else if( np->sockfd == target_fd && np->owner == pid && np->status == CLOSE_WAIT) {
          np->bound = 0;
          /* Build FIN packet here */
          Packet* pkt = this->allocatePacket(54);

          uint32_t myip = (np->srcip)? np->srcip : ADDR(ip_buffer);
          build_packet(pkt, myip, np->srcport, np->destip, np->destport, np->seq, 0, (WIN | FIN), 51200);

          this->sendPacket("IPv4", pkt);
          np->status = LAST_ACK;
          np->uuid = syscallUUID;
          if(DEBUG) node_dump(*np);
          returnSystemCall(syscallUUID, success); 
          break;
        }else if(np->sockfd == target_fd && np->owner == pid)
          np->bound = 0;
       }  
         
    break;
    }
	case READ:
    {
		//this->syscall_read(syscallUUID, pid, param.param1_int, param.param2_ptr, param.param3_int);
    int connfd = param.param1_int;
    char* buf = (char *) param.param2_ptr;
    size_t len = (size_t) param.param3_int;

    // printf("[READ] Requesting len = %d BYTES\n", (int) len);

    // Step 1. find the socket by <sockfd, pid>
    list<node>::iterator np;
    
    for(np = socklist.begin(); np != socklist.end(); ++np) {
      if( np->sockfd == connfd && np->owner == pid ) {
        if( np->status == ESTAB) { // or np->status == CLOSE_WAIT ) { 
          if( np->recv_len < len ) {  // syscall gets blocked 
            np->read_uuid = syscallUUID;
            np->read_buf = buf;
            np->read_len = len;
          } else {
            // startpos + len >= BUFSIZE
            if(np->start_r + len >= BUFSIZE) {
              size_t first_seg = BUFSIZE - np->start_r;
              size_t second_seg = len - first_seg;

              memcpy(buf, &(np->recv_buffer[np->start_r]), first_seg);
              memcpy(&buf[first_seg], np->recv_buffer, second_seg);
              returnSystemCall(syscallUUID, len);
            } else {
              memcpy(buf, &np->recv_buffer[np->start_r], len);
              returnSystemCall(syscallUUID, len);  
            }
            np->recv_len -= len;
            np->start_r += len;
            if(np->start_r >= BUFSIZE)
              np->start_r -= BUFSIZE;
          }
        } else {
          if(DEBUG) printf("reading on closed socket : return 0\n");
          returnSystemCall(syscallUUID, -1);
          //node_dump(*np);
        }


        // np = The socket
        /* Step 2. copy to the string by len bytes 
         * Step 3. set the start point (start_r) of the socket by n-bytes
         * Step 4. Return to syscallUUID (n BYTES read)
         */      


        break;  
      }
    }



		break;
    }
	case WRITE:
  {
    int connfd = param.param1_int;
    char* buf = (char *) param.param2_ptr;
    size_t len = (size_t) param.param3_int;

    if(DEBUG) printf("[WRITE] %d bytes on %d", len, connfd);
    list<node>::iterator np;
    
    for(np = socklist.begin(); np != socklist.end(); ++np) {
      if(np->sockfd == connfd && np->owner == pid) {
        if(np->status == ESTAB) { // the socket

          if(np->send_len > 0) {

          if(np->send_len < len)  // requested send length > remaining buffer
            len = np->send_len;

          if(DEBUG) printf("[WRITE] len = %d, [%d/%d] BYTES remaining (start_s=%d, end_s=%d)\n",len, np->send_len, BUFSIZE,np->start_s, np->end_s);
          if(np->start_s + len >= BUFSIZE) {
            size_t first_seg = BUFSIZE - np->start_s;
            size_t second_seg = len - first_seg;
            memcpy(&(np->send_buffer[np->start_s]), buf, first_seg);
            memcpy(np->send_buffer, &buf[first_seg], second_seg);
            returnSystemCall(syscallUUID, len);
          } else {
            memcpy(&np->send_buffer[np->start_s], buf, len);
            returnSystemCall(syscallUUID, len);
          }
          np->start_s += len;
          np->send_len -= len;
          if(np->start_s >= BUFSIZE)
            np->start_s -= BUFSIZE;

           /* Actual packet is sent here : initiate transfer */          
        if(!np->sending) {
          np->sending = true;
          send_segments(*np, (np->cwnd)/MSS);
        } // if(!np->sending)
        } // if(np->send_len > 0)
        else {
          /* remaining sender buffer is full(np->send_len = 0) */
          np->write_uuid = syscallUUID;
          np->write_len = len;
          memcpy(np->write_buf, buf, len);
        }

        } else {
          // Writing on closed socket should fail
          returnSystemCall(syscallUUID, -1);
        }
        break;
      }
    }
    }
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
      if( (node_=getnodebysockfd(socklist, targetfd, pid)) == socklist.end() )
        returnSystemCall(syscallUUID, -1);  // socket not found!

      int newport = allocate_port(socklist);
      set_srcaddr(*node_, ntohl(*(uint32_t*)ip_buffer), newport);
      set_destaddr(*node_, addr, ntohs(sain->sin_port));
      setbound(*node_, 1);
      setuuid(*node_, (int) syscallUUID);
      set_addrlen(*node_, addrlen); 
      set_owner(*node_, pid);
      free(ip_buffer);
    
      // SEND SYN packet to server
      int seq = rand();
      setseq(*node_, seq);
      setack(*node_, 0);

      // (PACKET SENT HERE)
      Packet* syn = allocatePacket(54);

      build_packet(syn, node_->srcip, node_->srcport, node_->destip, node_->destport, node_->seq, 0, (WIN | SYN), 51200);
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
    if( (np=getnodebysockfd(socklist,sockfd,pid)) == socklist.end() )
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
    
    for(np = socklist.begin(); np != socklist.end(); ++np){
      if(np->sockfd==listenfd && np->status==LISTENING)
        break;
    }
    int accept_port = np->srcport;

    for(np = socklist.begin(); np!=socklist.end(); ++np){
      
      // Connection established but not yet returned
      if( (np->srcport==accept_port || !(np->srcport)) && np->status!=LISTENING && (np->used != 1) && (np->owner == pid)){

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

        if(DEBUG) printf("accept(%d) returns %d to caller %d\n", listenfd, np->sockfd, pid);
        returnSystemCall(syscallUUID, np->sockfd);
        break;
      }
    }
    if(np == socklist.end()){
      queue_elem_ waitnode;
      //memset(&waitnode, 0, sizeof(queue_elem_));
      waitnode.uuid = syscallUUID;
      waitnode.pid = pid;
      waitnode.sockfd = listenfd;
      waitnode.sa = sa;
      waitnode.slen = slen;
      
      accept_queue.push_back(waitnode);      
      break;
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
        if(np->sockfd == targetfd && np->owner == pid) {
          if(np->bound){
            returnSystemCall(syscallUUID,-1);
            break;
          }
          set_sockfd(*np, targetfd);
          set_srcaddr(*np, ip_addr, port_);
          setbound(*np, 1);
          set_addrlen(*np, socklen_);
          returnSystemCall(syscallUUID,0);
          break;
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


      if(DEBUG){
        printf("getsockname(fd=%d, pid=%d)\n",targetfd, pid);
        traverse(socklist);
      }
      //x01

      for(np=socklist.begin(); np!=socklist.end(); ++np){
        if(np->sockfd == targetfd && np->owner == pid){   // socket descriptor found!
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

    list<node>::iterator np = getnodebysockfd(socklist, targetfd, pid);
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
  /* For part 1, 2-1, and 2-2 */
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
  uint32_t src_ip_ = ntohl(*(uint32_t *)src_ip);
  uint32_t dest_ip_ = ntohl(*(uint32_t *)dest_ip);
  uint32_t seq_ = ntohl(*(uint32_t*)seq);
  uint32_t ack_ = ntohl(*(uint32_t*)ack);
  

  /* For part 3-1 and 3-2 */
  uint8_t payload_len_[2]; // payload size = TCP payload + 
  packet->readData(0x10, payload_len_, 2);  
  uint32_t payload_len = ntohs(*(uint16_t *) payload_len_) - 40;
  



  list<node>::iterator np;

  
  uint16_t flags = 0x3f & ntohs(*(uint16_t *)hdr_flag_);
  
  for(np = socklist.begin(); np != socklist.end(); ++np){
    if( (np->srcip==ADDR(dest_ip) || !np->srcip) &&  (np->srcport == PORT(dest_port) || !np->srcport) && np->status==LISTENING)
      break;
  }
  int sockfd = -1;
  if(np != socklist.end())
    sockfd = np->sockfd;

  if(flags == SYN){  // CASE SYN
    size_t limit = np->backlog;
    if(!in_list(backlog_list, sockfd, ADDR(src_ip), PORT(src_port))){
      node newnode; // = (node *) malloc(sizeof(node));
      memcpy(&newnode, &*np, sizeof(node));
      newnode.destip = ADDR(src_ip);
      newnode.destport = PORT(src_port);
      newnode.seq = rand();
      newnode.ack = ntohl((*(uint32_t*)seq))+1;
      newnode.status = SYN_RCVD;

      if(num_of_nodes(backlog_list, sockfd, newnode.owner) < limit){   // within limit
        /* Put in backlog list */
        backlog_list.push_back(newnode);
        
        /* send SYNACK */
        Packet* synack = this->allocatePacket(54);
        build_packet(synack, ADDR(dest_ip), PORT(dest_port), ADDR(src_ip), PORT(src_port), newnode.seq, newnode.ack, (WIN | (SYN | ACK)), 51200);
        this->sendPacket("IPv4", synack);
       }
    }

  }
  else if(flags == ACK) { // CASE ACK simply POPs out the element for srcip:srcport 
    for(np=backlog_list.begin(); np!=backlog_list.end(); ++np){ // cx01
      if(np->sockfd == sockfd && (np->destip==ADDR(src_ip) || !np->srcip) && (np->destport==PORT(src_port) || !np->srcport)){
        if(np->status == SYN_RCVD){
        // is the node found?
        np->seq = ack_;
        np->ack = seq_ +1;
        node newnode;
        memcpy(&newnode, &*np, sizeof(node));
        newnode.status = ESTAB;
        backlog_list.erase(np);

        if(!accept_queue.size()){
          newnode.sockfd = createFileDescriptor(newnode.owner);
          check_fd(newnode.owner, newnode.sockfd);
          socklist.push_front(newnode);
          break;
        } else {
          list<queue_elem_>::iterator qp = accept_queue.begin();
          int new_fd = createFileDescriptor(qp->pid);
          check_fd(qp->pid, new_fd);

          if(new_fd == -1){
            accept_queue.pop_front();
            returnSystemCall(qp->uuid, -1);
            break;
          }
          newnode.sockfd = new_fd;
          newnode.used = 1;
          newnode.owner = qp->pid;
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
     // PART socklist :: ACK arrival
      for(np=socklist.begin(); np!=socklist.end(); ++np){
        if(np->destip==src_ip_ && np->destport==src_port_ && np->status != LISTENING){

          np->ack = seq_;
          if(np->status == ESTAB) {
            if(payload_len) {
              np->ack += (payload_len);
              /* For part 3-1 and 3-2
               * 1. store in socket buffer, if receiver buffer has enough space.
               * 2. increment ACK by (len) bytes
               * 3. send ACK back
               */

              // Send ACK for the packet
              uint8_t actual_data[512];

              packet->readData(0x36, actual_data, payload_len);

              if(np->end_r + payload_len >= BUFSIZE){  // split the string into 2 substrings
                size_t first_seg = BUFSIZE - np->end_r;
                size_t second_seg = payload_len - first_seg;

                if(DEBUG) printf("PAYLOAD %d BYTES = %d(start=%d) + %d(start=%d) \n", payload_len, first_seg, np->end_r, second_seg, 0);
                memcpy( &(np->recv_buffer[np->end_r]), actual_data, first_seg);
                memcpy( np->recv_buffer, &actual_data[first_seg], second_seg);
              } else {
                memcpy( &np->recv_buffer[np->end_r], actual_data, payload_len);
              }

              np->recv_len += payload_len;
              np->end_r += payload_len;


              if(np->read_uuid >= 0 && (np->recv_len >= np->read_len)) {
                // TODO-0x02
                // startpos + len >= BUFSIZE

                if(np->start_r + np->read_len >= BUFSIZE) {
                  size_t first_seg = BUFSIZE - np->start_r;
                  size_t second_seg = np->read_len - first_seg;

                  memcpy(np->read_buf, &(np->recv_buffer[np->start_r]), first_seg);
                  memcpy(np->read_buf + first_seg, np->recv_buffer, second_seg);
                  returnSystemCall(np->read_uuid, np->read_len);
                  np->read_uuid = -1;
                  if(DEBUG) printf("[READ] recv_buf[%d:%d] + recv_buf[%d:%d] \n", np->start_r-1, BUFSIZE-1, 0, second_seg-1);
                } else {
                  memcpy(np->read_buf, &np->recv_buffer[np->start_r], np->read_len);
                  returnSystemCall(np->read_uuid, np->read_len);  
                  np->read_uuid = -1;
                  if(DEBUG) printf("[READ] recv_buf[%d:%d] \n", np->start_r, np->start_r + np->read_len-1);
                }


                np->start_r += np->read_len;
                np->recv_len -= np->read_len;

                if(np->start_r >= BUFSIZE)
                  np->start_r -= BUFSIZE;

              }

              if(np->end_r >= BUFSIZE)
                np->end_r -= BUFSIZE;
              
              Packet* ack_pkt = this->allocatePacket(54);
              build_packet(ack_pkt, ADDR(dest_ip), PORT(dest_port), ADDR(src_ip), PORT(src_port), np->seq, np->ack, WIN | ACK, 51200 - np->recv_len);
              this->sendPacket("IPv4", ack_pkt);
            } else {

            /* Check for duplicate ACK */
              if(ack_ == np->recentACK) {
                if(DEBUG) printf("[ACK arrived] ACK = %d, recent ACK = %d\n", ack_, np->recentACK);
                np->dupACKcnt++;
              } else {
                if(DEBUG) printf("[ACK arrived] ACK = %d(new), recent ACK = %d\n", ack_, np->recentACK);
                np->dupACKcnt = 0;
              }
            np->recentACK = ack_;


            np->cwnd += ((np->cwnd <= np->ssthresh)? MSS:MSS*MSS/(np->cwnd));

            if(np->cwnd > MSS) printf("[ACK arrived] current cwnd = %dMSS\n", np->cwnd/MSS);

            if(np->sending) {
              send_segments(*np, 2);

            
            /* unblock write() */
            if(np->write_uuid > 0) {
              printf("ack arrived: freeing %d bytes of buffer (caller = %d)\n", MSS, np->write_uuid);
              if(np->start_s + MSS >= BUFSIZE) {
                memcpy(&np->send_buffer[np->start_s], np->write_buf, BUFSIZE - np->start_s);
                memcpy(np->send_buffer, &np->write_buf[BUFSIZE - np->start_s], MSS + np->start_s - BUFSIZE);
              } else {
                memcpy(&np->send_buffer[np->start_s], np->write_buf, MSS);
              }
              np->start_s += MSS;
              if(np->start_s >= BUFSIZE) np->start_s -= BUFSIZE;
              np->send_len -= MSS;
              returnSystemCall(np->write_uuid, MSS);

           }
            np->write_uuid = -1;


            }
            }


          } else if(np->status == FIN_WAIT_1) {
            np->ack = seq_ + 1;
            /* Change the state of socket to FIN_WAIT_2 */
            np->status = FIN_WAIT_2;
            break;
          } else if(np->status == LAST_ACK) {
            np->ack = seq_ + 1;
            /* Return exit status back to syscallUUID */
            returnSystemCall(np->uuid,0);
            /* Clean up the socket descriptor */
            socklist.erase(np);
          } else if(np->status == CLOSING){
            // np->ack = seq_ + 1;
            // change state to TIMED_WAIT
            // and send nothing
            np->status = TIMED_WAIT;
            // TimerModule::addTimer(&*np, TimeUtil::makeTime(120,TimeUtil::TimeUnit::SEC))
            returnSystemCall(np->uuid, 0);
          }
          break;
        }
      }

  }
  // SYN,ACK
  else if(flags == (SYN | ACK)){
  // send ACK to server
  for(np = socklist.begin(); np != socklist.end(); ++np){
    if(np->srcport==dest_port_ && np->status==SYN_SENT){
      if(DEBUG) node_dump(*np);
      // change the socket state to ESTAB
      np->status = ESTAB;

      // set seq, ack, and send ACK back
      np->seq = ack_;
      np->ack = seq_+1;

      Packet* estab_syn = this->allocatePacket(54);
      build_packet(estab_syn, ADDR(dest_ip), PORT(dest_port), ADDR(src_ip), PORT(src_port), np->seq, np->ack, (WIN | ACK), 51200);
      this->sendPacket("IPv4",estab_syn);

      returnSystemCall(np->uuid, 0);
    }
  }
  }
  // FIN
  else if(flags == FIN){
    list<node>::iterator np;

    for(np=socklist.begin(); np!=socklist.end(); ++np){
      if(np->destport==PORT(src_port) && np->destip==ADDR(src_ip) && np->status == ESTAB){
        Packet* pkt = this->allocatePacket(54);
        build_packet(pkt, ADDR(dest_ip), PORT(dest_port), ADDR(src_ip), PORT(src_port), np->seq, (np->ack = seq_+1), (WIN | ACK), 51200);
        this->sendPacket("IPv4",pkt);
        np->status = CLOSE_WAIT;
        if(np->read_uuid >= 0) {
          if(DEBUG) printf("[FIN arrival] <EOF> Return %d bytes to application\n", np->recv_len);
          if(np->start_r + np->recv_len >= BUFSIZE) {
            size_t first_seg = BUFSIZE - np->start_r;
            size_t second_seg = np->recv_len - first_seg;

            memcpy(np->read_buf, &(np->recv_buffer[np->start_r]), first_seg);
            memcpy(np->read_buf + first_seg, np->recv_buffer, second_seg);
            returnSystemCall(np->read_uuid, np->recv_len);
            np->read_uuid = -1;
          } else {
            memcpy(np->read_buf, &np->recv_buffer[np->start_r], np->recv_len);
            returnSystemCall(np->read_uuid, np->recv_len);  
            np->read_uuid = -1;
            if(DEBUG) printf("[READ] recv_buf[%d:%d] \n", np->start_r, np->start_r + np->read_len-1);
          }
        }
        break;
      } else if(np->destport==PORT(src_port) && np->destip==ADDR(src_ip) && np->status==FIN_WAIT_2){
        Packet* pkt = this->allocatePacket(54);
        build_packet(pkt, ADDR(dest_ip), PORT(dest_port), ADDR(src_ip), PORT(src_port), ++np->seq, (np->ack = seq_+1), (WIN | ACK), 51200);
        this->sendPacket("IPv4",pkt);

        /* TIMED_WAIT timer code */
        np->status = TIMED_WAIT;
        // TimerModule::addTimer(&*np, TimeUtil::makeTime(120,TimeUtil::TimeUnit::SEC));
        returnSystemCall(np->uuid, 0);
        break;

      } else if(np->destport==PORT(src_port) && np->destip==ADDR(src_ip) && np->status == FIN_WAIT_1){
        Packet* pkt = this->allocatePacket(54);
        build_packet(pkt, ADDR(dest_ip), PORT(dest_port), ADDR(src_ip), PORT(src_port), ++np->seq, (np->ack = seq_+1), (WIN | ACK), 51200);
        this->sendPacket("IPv4",pkt);
        np->status = CLOSING;
        break;
      } else if(np->destport==PORT(src_port) && np->destip==ADDR(src_ip)) {
        break;
      }
    }

    // find the socket 'node' and MUX
    // by np->status

    // case FIN_WAIT_2 : client side(active close)
    // change state : FIN_WAIT_2 -> TIMED_WAIT
    // send ACK to server

    // case ESTAB = server side(passive close)
    // change state : ESTAB -> CLOSE_WAIT
    // send ACK to client

  }
  this->freePacket(packet);
}
void TCPAssignment::timerCallback(void* payload)
{

  node* np = (node *) payload;
  np->status = CLOSED;

  int sockfd = np->sockfd;
  int owner = np->owner;

  list<node>::iterator nq;
  for(nq = socklist.begin(); nq != socklist.end(); ++nq){
    if(nq->sockfd == sockfd && nq->owner == owner)
      break;
  }
  if(nq == socklist.end() && ( nq->sockfd != sockfd || nq->owner != owner))
    return;

  returnSystemCall(nq->uuid, 0);
  socklist.erase(nq);
}


}

/* Helper subroutine definitions */
void build_packet_data(E::Packet* pkt, uint32_t srcip, uint16_t srcport, uint32_t destip, uint16_t destport, uint32_t seq, uint32_t ack, uint16_t hdr_flag, uint16_t winsize, uint8_t* data, size_t datalen) {
  uint8_t tcp_seg[20 + datalen];
  uint32_t srcip_ = htonl(srcip);
  uint32_t destip_ = htonl(destip);
  uint16_t srcport_ = htons(srcport);
  uint16_t destport_ = htons(destport);
  uint32_t seq_ = htonl(seq);
  uint32_t ack_ = htonl(ack);
  uint16_t hdr_flag_ = htons(hdr_flag);
  uint16_t wsize = htons(winsize);

  memset(&tcp_seg, 0, 20);

  memcpy(&tcp_seg[0], &srcport_, 2);
  memcpy(&tcp_seg[2], &destport_, 2);
  memcpy(&tcp_seg[4], &seq_, 4);
  memcpy(&tcp_seg[8], &ack_, 4);
  memcpy(&tcp_seg[12], &hdr_flag_, 2);
  memcpy(&tcp_seg[14], &wsize, 2);

  memcpy(&tcp_seg[20], data, datalen);
  uint16_t csum = htons(0xffff - E::NetworkUtil::tcp_sum(srcip_, destip_, tcp_seg, 20 + datalen));
  memcpy(&tcp_seg[16], &csum, 2);

  pkt->writeData(0x22, tcp_seg, 20 + datalen);
  pkt->writeData(0x1a, &srcip_, 4);
  pkt->writeData(0x1e, &destip_, 4);
}

// build_packet : build a packet using given info and pointer.
void build_packet(E::Packet* pkt, uint32_t srcip, uint16_t srcport, uint32_t destip, uint16_t destport, uint32_t seq, uint32_t ack, uint16_t hdr_flag, uint16_t winsize){
  // Assume all parameters are in host byte ordering
  uint8_t tcp_seg[20];
  uint32_t srcip_ = htonl(srcip);
  uint32_t destip_ = htonl(destip);
  uint16_t srcport_ = htons(srcport);
  uint16_t destport_ = htons(destport);
  uint32_t seq_ = htonl(seq);
  uint32_t ack_ = htonl(ack);
  uint16_t hdr_flag_ = htons(hdr_flag);
  uint16_t wsize = htons(winsize);

  memset(&tcp_seg, 0, 20);

  memcpy(&tcp_seg[0], &srcport_, 2);
  memcpy(&tcp_seg[2], &destport_, 2);
  memcpy(&tcp_seg[4], &seq_, 4);
  memcpy(&tcp_seg[8], &ack_, 4);
  memcpy(&tcp_seg[12], &hdr_flag_, 2);
  memcpy(&tcp_seg[14], &wsize, 2);

  uint16_t csum = htons(0xffff - E::NetworkUtil::tcp_sum(srcip_, destip_, tcp_seg, 20));
  memcpy(&tcp_seg[16], &csum, 2);

  pkt->writeData(34, tcp_seg, 20);
  pkt->writeData(26, &srcip_, 4);
  pkt->writeData(30, &destip_, 4);
}


/* Debug helper functions */
void check_fd(int pid, int fd){
  if(DEBUG)
    printf("createFileDescriptor(pid = %d) : fd = %d\n", pid, fd);
}

void state(node nd){
  switch(nd.status){
    case ESTAB:
      cout << "ESTAB" ;
      break;
    case CLOSED:
      cout << "CLOSED" ;
      break;
    case LISTENING:
      cout << "LISTEN";
      break;
    case SYN_SENT:
      cout << "SYN_SENT";
      break;
    case SYN_RCVD:
      cout << "SYN_RCVD";
      break;
    case FIN_WAIT_1:
      cout << "FIN_WAIT_1";
      break;
    case FIN_WAIT_2:
      cout << "FIN_WAIT_2";
      break;
    case CLOSE_WAIT:
      cout << "CLOSE_WAIT";
      break;
    case LAST_ACK:
      cout << "LAST_ACK";
      break;
    case CLOSING:
      cout << "CLOSING";
      break;
    case TIMED_WAIT:
      cout << "TIMED_WAIT";
      break;
    default:
      cout << "Unknown state : " << nd.status;
      break;
  }
}

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
void set_owner(node& nd, int pid){
  nd.owner = pid;
}
void node_dump(node nd){
  cout << "[FD = " << nd.sockfd << ", PID = " << nd.owner << "] ";
  state(nd);
  cout<<endl;
  uint32_t src = nd.srcip;
  uint32_t dst = nd.destip;
  printf("[source] %u.%u.%u.%u:%d => [dest] %u.%u.%u.%u:%d\n", (src>>24)&0xff, (src>>16)&0xff, (src>>8)&0xff, src&0xff, nd.srcport, (dst>>24)&0xff, (dst>>16)&0xff, (dst>>8)&0xff, dst&0xff, nd.destport);

}
/* <NODE>
 * sockfd / status / srcip srcport seq / destip destport ack / bound
 */
void node_init(node& nd){
  nd.backlog=0;
  nd.used=0;
  nd.bound=0;
  nd.ssthresh = 10*MSS;
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
void traverse(list<node> nd){
  for(list<node>::iterator np = nd.begin(); np!=nd.end(); ++np)
    node_dump(*np);
}

// List traversal for backlog
// how many 'sockfd's in backlog list?
int num_of_nodes(list<node>& backlog_list, int sockfd_, int pid){
  list<node>::iterator np;
  int cnt=0;
  for(np=backlog_list.begin(); np!=backlog_list.end(); ++np){
    if(np->sockfd == sockfd_ && np->owner == pid)
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
list<node>::iterator getnodebysockfd(list<node>& sl, int sockfd_, int pid){
  list<node>::iterator np;
  for(np = sl.begin(); np != sl.end(); ++np){
    if(np->sockfd == sockfd_ && np->owner == pid)
      return np;
  }
  return sl.end(); 
}


