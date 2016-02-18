/***********************************************
*
*Filename: packetSniff.c
*
*@author: Xianghang Mi
*@email: mixianghang@outlook.com
*@description: ---
*Create: 2015-12-27 16:23:56
# Last Modified: 2016-02-18 18:00:39
************************************************/
#include "packetSniff.h"

pcap_t *handle;
char *logFilePath = NULL;
struct SniffPanel panel;
void got_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *packet);

void signalHandle(int sigNum) {
  switch(sigNum) {
	case SIGINT: {
	  printf("nihao, sigsint\n");
	  if (handle) {
		printf("interrupt\n");
		pcap_breakloop(handle);
	  }
	  break;
	}
	case SIGCONT: {
	  printf("welcome sigcont\n");
	  FILE *logFd = fopen(logFilePath, "a");
	  if (!logFd) {
		printf("logfile %s doesn't exist\n", logFilePath);
		exit(1);
	  }
	  char buffer[1024] = {0};
	  snprintf(buffer, sizeof buffer - 1, "%d\t%d\t", panel.payloadSize / 1024, panel.packetNum);
	  printf("%s\n", buffer);
	  if (fwrite(buffer, sizeof(char), strlen(buffer), logFd) != strlen(buffer)) {
		printf("write to file %s failed\n", logFilePath);
		exit(1);
	  }
	  fclose(logFd);
	  break;
	}
	default: {
	  break;
	}
  }
}

int main(int argc, char *argv[]){
  if (argc < 5) {
	fprintf(stderr, "Usage: ./packetSniff interface filter_expression payloadFile logFilePath\n");
	return 1;
  }
  signal(SIGINT, signalHandle);
  signal(SIGCONT, signalHandle);
  char *dev = argv[1];
  char *payloadFilePath = argv[3];
  logFilePath = argv[4];
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

  FILE *logFd = fopen(logFilePath, "a");
  if (!logFd) {
	printf("logfile %s doesn't exist\n", logFilePath);
	return 1;
  }
  char buffer[1024] = {0};
  snprintf(buffer, sizeof buffer - 1, "%d\t%d\n", panel.payloadSize / 1024, panel.packetNum);
  if (fwrite(buffer, sizeof(char), strlen(buffer), logFd) != strlen(buffer)) {
	printf("write to file %s failed\n", logFilePath);
	return 1;
  }
  fclose(logFd);
  printf("response code of loop is %d\n", response_loop);
  printf("finish with len %d and packet num %d\n", panel.payloadSize / 1024, panel.packetNum);
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
  char logMsg[1024] = {0};
  u_int size_ip;
  u_int size_tcp;
  ethernet = (struct sniff_ethernet*)(packet);
  ip = (struct sniff_ip*)(packet + SIZE_ETHERNET);
  size_ip = IP_HL(ip)*4;
  if (size_ip < 20) {
	  memset(logMsg, 0, sizeof logMsg);
	  snprintf(logMsg, sizeof logMsg - 1, "   * Invalid IP header length: %u bytes\n", size_ip);
	  fprintf(stderr, "%s", logMsg);
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
	  memset(logMsg, 0, sizeof logMsg);
	  snprintf(logMsg, sizeof logMsg - 1, "   * Invalid TCP header length: %u bytes\n", size_tcp);
	  fprintf(stderr, "%s", logMsg);
	  return;
  }
  payload = (char *)(packet + SIZE_ETHERNET + size_ip + size_tcp);
  int payloadLen = header->len - SIZE_ETHERNET - size_tcp - size_ip;
  memset(logMsg, 0, sizeof logMsg);
  snprintf(logMsg, sizeof logMsg - 1, "source: %s:%d, dst: %s:%d\n", srcIpStr, srcPort, dstIpStr, dstPort);
  //printf("%s", logMsg);
  memset(logMsg, 0, sizeof logMsg);
  snprintf(logMsg, sizeof logMsg - 1, "packetNum: %d, len: %d, ethernet header: %d, ip header: %d, tcp header: %d, payload: %d\n", panel->packetNum, header->len, SIZE_ETHERNET, size_ip, size_tcp, payloadLen);
  printf("%s", logMsg);
  panel->payloadSize += payloadLen;
  int writeToFileLen = 0;
  //printf("%s\n", payload);
  if (panel->payloadFile != NULL) {
	//writeToFileLen = fwrite(payload, sizeof(char), payloadLen, panel->payloadFile); 
  }
  memset(logMsg, 0, sizeof logMsg);
  snprintf(logMsg, sizeof logMsg - 1, "sum size is %d, write to file: %d\n", panel->payloadSize, writeToFileLen); 
  //printf("%s", logMsg);
  return;
}

/**
compile filter expression, open sniff handler
*/
int initSniff(SniffPanel *panel) {
  pcap_t *handle;
  char *dev = panel->device;
  char errbuf[1024] = {0};
  struct bpf_program fp;		/* The compiled filter */
  char *filter_exp = panel->filterExpression;	/* The filter expression */
  unsigned int liveReadTimeOut = panel->liveReadTimeOut;
  bpf_u_int32 mask;		/* Our netmask */
  bpf_u_int32 net;		/* Our IP */
  if (dev == NULL) {
	panel->cbWhenError(panel, 2, "device cannot be NULL\n", panel->errorArgs);
	return 2;
  }
  /* Find the properties for the device */
  if (pcap_lookupnet(dev, &net, &mask, errbuf) == -1) {
	panel->cbWhenError(panel, 2, errbuf, panel->errorArgs);
	net = 0;
	mask = 0;
  }
  /* Open the session in promiscuous mode */
  handle = pcap_open_live(dev, BUFSIZ, 0, liveReadTimeOut, errbuf);
  //handle = pcap_open_live(dev, 1000000000, 0, 0, errbuf);
  if (handle == NULL) {
	panel->cbWhenError(panel, 2, errbuf, panel->errorArgs);
	return 2;
  }
  /* Compile and apply the filter */
  if (pcap_compile(handle, &fp, filter_exp, 0, net) == -1) {
	memset(errbuf, 0, sizeof errbuf);
	snprintf(errbuf, sizeof errbuf -1, "Couldn't parse filter %s: %s\n", filter_exp, pcap_geterr(handle));
	panel->cbWhenError(panel, 2, errbuf, panel->errorArgs);
	return 2;
  }

  if (pcap_setfilter(handle, &fp) == -1) {
	memset(errbuf, 0, sizeof errbuf);
	snprintf(errbuf, sizeof errbuf -1, "Couldn't install filter %s: %s\n", filter_exp, pcap_geterr(handle));
	panel->cbWhenError(panel, 2, errbuf, panel->errorArgs);
	return 2;
  }
  panel->handle = handle;
  panel->net = net;
  panel->mask = mask;
  panel->fp = fp;
  return 0;
}

/** run the sniffer*/
int runSniff(SniffPanel *panel) {
  printf("run sniff %s\n", __func__);
  char errbuf[1024] = {0};
  pcap_t *handle = panel->handle;
  if (handle == NULL) {
	memset(errbuf, 0, sizeof errbuf);
	snprintf(errbuf, sizeof errbuf -1, "handle cannot be NULL in %s %s %d", __FILE__, __func__, __LINE__);
	panel->cbWhenError(panel, 2, errbuf, panel->errorArgs);
	return 2;
  }
  int response_loop = pcap_loop(handle, -1, got_packet, (u_char *)panel);
  if (response_loop == -1) {
	memset(errbuf, 0, sizeof errbuf);
	snprintf(errbuf, sizeof errbuf -1, "pcap_loop returns error of %s in %s %s %d", pcap_geterr(handle), __FILE__, __func__, __LINE__);
	panel->cbWhenError(panel, 2, errbuf, panel->errorArgs);
	return -1;
  } else if (response_loop == -2) {
	panel->cbAfterSniff(panel, panel->afterSniffArgs);
	panel->isStopped = 1;
  }
  return 0;
}

/** stop the sniffer*/
int stopSniff(SniffPanel *panel) {
  pcap_t * handle = panel->handle;
  if (handle) {
	pcap_breakloop(handle);
	while (1) {
	  if (panel->isStopped) {
		return 0;
	  }
	  sleep(1);
	}
  }
  return 0;
}

/* clean the sniffer*/
int cleanSniff(SniffPanel *panel) {
  if (panel->handle) {
	pcap_close(panel->handle);
	panel->handle = NULL;
  }
  return 0;
}
