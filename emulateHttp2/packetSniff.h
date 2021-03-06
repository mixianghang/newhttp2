/***********************************************
*
*Filename: packetSniff.h
*
*@author: Xianghang Mi
*@email: mixianghang@outlook.com
*@description: ---
*Create: 2015-12-29 08:43:16
# Last Modified: 2016-02-18 17:11:31
************************************************/
#include <pcap/pcap.h>
#include <stdio.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

typedef struct SniffPanel {
  char * device;
  char * filterExpression;
  unsigned int liveReadTimeOut;
  char * payloadFilePath;
  FILE * payloadFile;
  unsigned int packetNum;
  unsigned int payloadSize;
  unsigned short isStopped;
  void *afterSniffArgs;
  void (*cbAfterSniff)(const struct SniffPanel *panel, void * userArgs); 
  void *errorArgs;
  void (*cbWhenError)(const struct SniffPanel *panel,const int errorCode, char * errorMsg, void * userArgs); 
  void (*cbLog)(const char * logMsg);
  bpf_u_int32 mask;		/* Our netmask */
  bpf_u_int32 net;		/* Our IP */
  struct bpf_program fp;		/* The compiled filter */
  pcap_t *handle;
} SniffPanel;
/* Ethernet addresses are 6 bytes */
#define ETHER_ADDR_LEN	6

/* Ethernet header */
struct sniff_ethernet {
	u_char ether_dhost[ETHER_ADDR_LEN]; /* Destination host address */
	u_char ether_shost[ETHER_ADDR_LEN]; /* Source host address */
	u_short ether_type; /* IP? ARP? RARP? etc */
};

/* IP header */
struct sniff_ip {
	u_char ip_vhl;		/* version << 4 | header length >> 2 */
	u_char ip_tos;		/* type of service */
	u_short ip_len;		/* total length */
	u_short ip_id;		/* identification */
	u_short ip_off;		/* fragment offset field */
#define IP_RF 0x8000		/* reserved fragment flag */
#define IP_DF 0x4000		/* dont fragment flag */
#define IP_MF 0x2000		/* more fragments flag */
#define IP_OFFMASK 0x1fff	/* mask for fragmenting bits */
	u_char ip_ttl;		/* time to live */
	u_char ip_p;		/* protocol */
	u_short ip_sum;		/* checksum */
	struct in_addr ip_src,ip_dst; /* source and dest address */
};
#define IP_HL(ip)		(((ip)->ip_vhl) & 0x0f)
#define IP_V(ip)		(((ip)->ip_vhl) >> 4)

/* TCP header */
typedef u_int tcp_seq;

struct sniff_tcp {
	u_short th_sport;	/* source port */
	u_short th_dport;	/* destination port */
	tcp_seq th_seq;		/* sequence number */
	tcp_seq th_ack;		/* acknowledgement number */
	u_char th_offx2;	/* data offset, rsvd */
#define TH_OFF(th)	(((th)->th_offx2 & 0xf0) >> 4)
	u_char th_flags;
#define TH_FIN 0x01
#define TH_SYN 0x02
#define TH_RST 0x04
#define TH_PUSH 0x08
#define TH_ACK 0x10
#define TH_URG 0x20
#define TH_ECE 0x40
#define TH_CWR 0x80
#define TH_FLAGS (TH_FIN|TH_SYN|TH_RST|TH_ACK|TH_URG|TH_ECE|TH_CWR)
	u_short th_win;		/* window */
	u_short th_sum;		/* checksum */
	u_short th_urp;		/* urgent pointer */
};

/**
compile filter expression, open sniff handler
*/
int initSniff(SniffPanel *panel);

/** run the sniffer*/
int runSniff(SniffPanel *panel);

/** stop the sniffer*/
int stopSniff(SniffPanel *panel);

/* clean the sniffer*/
int cleanSniff(SniffPanel *panel);

