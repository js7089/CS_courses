/*
 * E_TCPAssignment.hpp
 *
 *  Created on: 2014. 11. 20.
 *      Author: Keunhong Lee
 */

#ifndef E_TCPASSIGNMENT_HPP_
#define E_TCPASSIGNMENT_HPP_


#include <E/Networking/E_Networking.hpp>
#include <E/Networking/E_Host.hpp>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <netinet/in.h>


#include <E/E_TimerModule.hpp>

/* Defining connection status */
enum conn_status {
    LISTEN,
    SYN_SENT,
    SYN_RCVD,
    ESTAB,
    CLOSED
};

/* Custom data structure "Node" */
struct DescriptorInfo {
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
    // BOUND?
    int bound;
};

typedef struct DescriptorInfo node;

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
