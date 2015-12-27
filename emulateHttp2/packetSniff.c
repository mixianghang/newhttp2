/***********************************************
*
*Filename: packetSniff.c
*
*@author: Xianghang Mi
*@email: mixianghang@outlook.com
*@description: ---
*Create: 2015-12-27 16:23:56
# Last Modified: 2015-12-27 16:45:51
************************************************/
#include <pcap.h>
#include <stdio.h>

int main(int artc, char *argv[]){
  pcap_t *handle;
  char *dev = argv[1];
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
  handle = pcap_open_live(dev, BUFSIZ, 0, 1000, errbuf);
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
  /* Grab a packet */
  packet = pcap_next(handle, &header);
  /* Print its length */
  printf("the length of the header is %d\n", header.len);
  /* pcap close the handle*/
  pcap_close(handle);
  return 0;
}
