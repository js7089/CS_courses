/*
 * E_TCPAssignment.hpp
 *
 *  Created on: 2014. 11. 20.
 *      Author: Keunhong Lee
 */

#ifndef E_TCPASSIGNMENT_HPP_
#define E_TCPASSIGNMENT_HPP_

#include <E/E_Common.hpp>
#include <E/Networking/E_Networking.hpp>
#include <E/Networking/E_Host.hpp>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <netinet/in.h>


#include <E/E_TimerModule.hpp>

#define BUFSIZE 51200

/* Defining connection status */
enum conn_status {
  /* project #2-1. connection setup */
  LISTENING,
  SYN_SENT,
  SYN_RCVD,
  ESTAB,
  CLOSED,
  /* project #2-2. connection teardown */
  FIN_WAIT_1,
  FIN_WAIT_2,
  TIMED_WAIT,
  CLOSING,
  CLOSE_WAIT,
  LAST_ACK
};

typedef struct data_ {
  int a;
  int b;
} data;


/* Custom data structure "Node" */
struct DescriptorInfo {

  DescriptorInfo() : used(0) { }

  int owner;

  // ALLOCATED FILE DESCRIPTOR
  int sockfd;
  // CONNECTION STATUS
  enum conn_status status;
  // SRC    
  uint32_t srcip;
  uint16_t srcport;
  uint32_t seq;   
  // DEST
  uint32_t destip;
  uint16_t destport;
  uint32_t ack;
  // PEER info
  socklen_t addrlen;
  // BOUND?
  int bound=0;
  // UUID : who is responsible for the socket?
  int uuid;
  // backlog : for listen(sockfd, backlog)
  int backlog;
  int used=0;

  uint8_t send_buffer[BUFSIZE];
  size_t start_s = 0;
  size_t end_s = 0;

  /* receiver buffer */
  uint8_t recv_buffer[BUFSIZE];
  size_t start_r = 0;
  size_t end_r = 0;
  size_t recv_len = 0;

  int read_uuid = 0;    // waiter for the receiver buffer
  char* read_buf;
  size_t read_len = 0;  // how many bytes requested

};

struct queue_elem {
  int uuid;
  int pid;
  int sockfd;
  struct sockaddr* sa;
  socklen_t* slen;
};


typedef struct DescriptorInfo node;
typedef struct queue_elem queue_elem_;

void set_sockfd(node& nd, int sockfd_);
void set_status(node& nd, conn_status stat_);
void set_srcaddr(node& nd, uint32_t srcip_, uint16_t srcport_);
void set_destaddr(node& nd, uint32_t destip_, uint16_t destport_);
void setbound(node& nd, int bound_);
void setuuid(node& nd, int uuid_);
void setseq(node& nd, int seq_);
void setack(node& nd, int ack_);
void set_addrlen(node& nd,socklen_t addrlen);
void set_backlog(node& nd, int backlog);
void incr_seq(node& nd);
void incr_ack(node& nd);
void set_owner(node& nd, int pid);
void hexdump(void* obj, size_t len);

namespace E
{

class TCPAssignment : public HostModule, public NetworkModule, public SystemCallInterface, private NetworkLog, private TimerModule
{
private:

private:
	virtual void timerCallback(void* payload) final;

public:
	TCPAssignment(Host* host);
	virtual void initialize();
	virtual void finalize();
	virtual ~TCPAssignment();
protected:
	virtual void systemCallback(UUID syscallUUID, int pid, const SystemCallParameter& param) final;
	virtual void packetArrived(std::string fromModule, Packet* packet) final;
};

class TCPAssignmentProvider
{
private:
	TCPAssignmentProvider() {}
	~TCPAssignmentProvider() {}
public:
	static HostModule* allocate(Host* host) { return new TCPAssignment(host); }
};

}


#endif /* E_TCPASSIGNMENT_HPP_ */
