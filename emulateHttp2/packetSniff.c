/***********************************************
*
*Filename: packetSniff.c
*
*@author: Xianghang Mi
*@email: mixianghang@outlook.com
*@description: ---
*Create: 2015-12-27 16:23:56
# Last Modified: 2015-12-28 10:39:54
************************************************/
#include <pcap.h>
#include <stdio.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <signal.h>
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

struct SniffPanel {
  char * payloadFilePath;
  FILE * payloadFile;
  unsigned int packetNum;
  unsigned int payloadSize;
};

pcap_t *handle;
struct SniffPanel panel;
void got_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *packet);

void intSignalHandle(int sigNum) {
  if (handle) {
	pcap_breakloop(handle);
  }
}

int main(int argc, char *argv[]){
  if (argc < 4) {
	fprintf(stderr, "Usage: ./packetSniff interface filter_expression payloadFile\n");
	return 1;
  }
  signal(SIGINT, intSignalHandle);
  char *dev = argv[1];
  char *payloadFilePath = argv[3];
  char errbuf[PCAP_ERRBUF_SIZE];
  struct bpf_program fp;		/* The compiled filter */
  char *filter_exp = argv[2];	/* The filter expression */
  bpf_u_int32 mask;		/* Our netmask */
  bpf_u_int32 net;		/* Our IP */
  struct pcap_pkthdr header;	/* The header that pcap gives us */
  const u_char *packet;		/* The actual packet */
  /* Find the properties for the device */
  if (pcap_lookupnet(dev, &net, &mask, errbuf) == -1) {
	fprintf(stderr, "Couldn't get netmask for device %s: %s\n", dev, errbuf);
	net = 0;
	mask = 0;
  }
  /* Open the session in promiscuous mode */
  handle = pcap_open_live(dev, BUFSIZ, 1, 0, errbuf);
  //handle = pcap_open_live(dev, 1000000000, 0, 0, errbuf);
  if (handle == NULL) {
	fprintf(stderr, "Couldn't open device %s: %s\n", dev, errbuf);
	return 2;
  }
  /* Compile and apply the filter */
  if (pcap_compile(handle, &fp, filter_exp, 0, net) == -1) {
	fprintf(stderr, "Couldn't parse filter %s: %s\n", filter_exp, pcap_geterr(handle));
	return 2;
  }

  if (pcap_setfilter(handle, &fp) == -1) {
	fprintf(stderr, "Couldn't install filter %s: %s\n", filter_exp, pcap_geterr(handle));
	return 2;
  }
  //int i = 0;
  //for (; i < 1000; i++) {
  //  /* Grab a packet */
  //  packet = pcap_next(handle, &header);
  //  /* Print its length */
  //  printf("the length of the header is %d\n", header.len);
  //  if (header.len > 54)
  //    printf("the content is %s\n", packet + 54);
  //}
  FILE *payloadFile = fopen(payloadFilePath, "w");
  if (!payloadFile) {
	fprintf(stderr, "open file %s failed\n", payloadFilePath);
	return 1;
  }
  payloadFile = freopen(payloadFilePath, "a", payloadFile);
  panel.payloadFilePath = payloadFilePath;
  panel.payloadFile = payloadFile;
  panel.packetNum = 0;
  panel.payloadSize = 0;
  int response_loop = pcap_loop(handle, -1, got_packet, (u_char *)&panel);
  /* pcap close the handle*/
  pcap_close(handle);
  fclose(payloadFile);
  printf("response code of loop is %d", response_loop);
  printf("finish with len %d and packet num %d\n", panel.payloadSize, panel.packetNum);
  return 0;
}

void got_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *packet) {
/* ethernet headers are always exactly 14 bytes */
#define SIZE_ETHERNET 14
  struct SniffPanel *panel = (struct SniffPanel *)args;
  panel->packetNum++;
  FILE *payloadFile = panel->payloadFile;
  const struct sniff_ethernet *ethernet; /* The ethernet header */
  const struct sniff_ip *ip; /* The IP header */
  const struct sniff_tcp *tcp; /* The TCP header */
  char *payload; /* Packet payload */
  u_int size_ip;
  u_int size_tcp;
  ethernet = (struct sniff_ethernet*)(packet);
  ip = (struct sniff_ip*)(packet + SIZE_ETHERNET);
  size_ip = IP_HL(ip)*4;
  if (size_ip < 20) {
	  printf("   * Invalid IP header length: %u bytes\n", size_ip);
	  return;
  }
  tcp = (struct sniff_tcp*)(packet + SIZE_ETHERNET + size_ip);
  char srcIpStr[15] = {0};
  char dstIpStr[15] = {0};
  int srcPort = ntohs(tcp->th_sport);
  int dstPort = ntohs(tcp->th_dport);
  inet_ntop(AF_INET, (void *) (&(ip->ip_src)), srcIpStr, 14);
  inet_ntop(AF_INET, (void *) (&(ip->ip_dst)), dstIpStr, 14);
  size_tcp = TH_OFF(tcp)*4;
  if (size_tcp < 20) {
	  printf("   * Invalid TCP header length: %u bytes\n", size_tcp);
	  return;
  }
  payload = (u_char *)(packet + SIZE_ETHERNET + size_ip + size_tcp);
  int payloadLen = header->len - SIZE_ETHERNET - size_tcp - size_ip;
  printf("source: %s:%d, dst: %s:%d\n", srcIpStr, srcPort, dstIpStr, dstPort);
  printf("packetNum: %d, len: %d, ethernet header: %d, ip header: %d, tcp header: %d, payload: %d\n", panel->packetNum, header->len, SIZE_ETHERNET, size_ip, size_tcp, payloadLen);
  panel->payloadSize += payloadLen;
  //printf("%s\n", payload);
  int writeToFileLen = fwrite(payload, sizeof(char), payloadLen, panel->payloadFile); 
  printf("sum size is %d, write to file: %d\n", panel->payloadSize, writeToFileLen); 
  //fprintf(payloadFile, "%s", payload);
  return;
}
