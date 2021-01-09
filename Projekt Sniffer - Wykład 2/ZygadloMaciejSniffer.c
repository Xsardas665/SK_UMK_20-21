#include <net/if.h>
#include <netinet/icmp6.h>
#include <netinet/if_ether.h>
#include <netinet/ip6.h>
#include <netinet/ip_icmp.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAXBUF 65536

int g_arp;
int g_dhcp;
int g_eth;
int g_icmp;
int g_icmp6;
int g_ip4;
int g_ip6;
int g_tcp;
int g_udp;

struct dhcphdr
{
	uint8_t opcode;
	uint8_t htype;
	uint8_t hlen;
	uint8_t hops;
	uint32_t xid;
	uint16_t secs;
	uint16_t flags;
	in_addr_t ciaddr;
	in_addr_t yiaddr;
	in_addr_t siaddr;
	in_addr_t giaddr;
};

void PacketARP(struct arphdr *arp);
void PacketDHCP(struct dhcphdr *dhcp);
void PacketETH(struct ethhdr *eth);
void PacketICMP(struct icmphdr *icmp);
void PacketICMP6(struct icmp6_hdr *icmp);
void PacketIP4(struct iphdr *ip);
void PacketIP6(struct ip6_hdr *ip);
void PacketTCP(struct tcphdr *tcp);
void PacketUPD(struct udphdr *udp);

void ParseArgs(int argc, char **argv);
void PrintHex(char *begin, unsigned char *hex, int size, char *end);
void PrintDec(char *begin, unsigned char *dec, int size, char *end);
void PrintError(char *function);

int main(int argc, char **argv)
{
	int sockfd, n;
	char buffer[MAXBUF];

	if (argc < 2)
	{
		printf("Usage:\n");
		printf("\t%s [INTERFACE] [OPTIONS]\n\n", argv[0]);
		printf("Avaible options:\n");
		printf("\t-a - ARP\n");
		printf("\t-d - DHCP\n");
		printf("\t-e - Ethernet Frame\n");
		printf("\t-i - ICMP\n");
		printf("\t-I - ICMP6\n");
		printf("\t-4 - IP4\n");
		printf("\t-6 - IP6\n");
		printf("\t-t - TCP\n");
		printf("\t-u - UDP\n");
		printf("No options - enable all.\n\n");
		exit(0);
	}

	ParseArgs(argc, argv);

	if ((sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1)
	{
		PrintError("socket()");
	}

	struct ifreq ifr;
	ifr.ifr_flags |= IFF_PROMISC;
	strncpy((char *)ifr.ifr_name, argv[optind], IF_NAMESIZE);

	if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) == -1)
	{
		PrintError("ioctl()");
	}

	while ((n = recv(sockfd, buffer, MAXBUF, 0)) != -1)
	{
		PacketETH((struct ethhdr *)buffer);
	}

	return 0;
}

void PacketARP(struct arphdr *arp)
{
	if (g_arp)
	{
		printf("------------ ARP ------------\n");
		printf("OPER: %d\n", ntohs(arp->ar_op));
		PrintHex("SHA: ", (unsigned char *)(arp + 1), arp->ar_hln, "\n");
		PrintDec("SPA: ", (unsigned char *)(arp + 1) + arp->ar_hln, arp->ar_pln, "\n");
		PrintHex("THA: ", (unsigned char *)(arp + 1) + arp->ar_hln + arp->ar_pln, arp->ar_hln, "\n");
		PrintDec("TPA: ", (unsigned char *)(arp + 1) + arp->ar_hln + arp->ar_pln + arp->ar_hln, arp->ar_pln, "\n");
		printf("---------- END ARP ----------\n");
	}
}

void PacketETH(struct ethhdr *eth)
{
	int type = ntohs(eth->h_proto);

	if (g_eth)
	{
		printf("---------- ETHERNET ----------\n");
		PrintHex("Src MAC: ", eth->h_source, 6, "\n");
		PrintHex("Dst MAC: ", eth->h_dest, 6, "\n");
		printf("Type: 0x%04X\n", type);
	}

	switch (type)
	{
	case ETH_P_ARP:
		PacketARP((struct arphdr *)(eth + 1));
		break;
	case ETH_P_IP:
		PacketIP4((struct iphdr *)(eth + 1));
		break;
	case ETH_P_IPV6:
		PacketIP6((struct ip6_hdr *)(eth + 1));
		break;
	}

	if (g_eth)
	{
		printf("-------- END ETHERNET --------\n\n");
	}
}

void PacketDHCP(struct dhcphdr *dhcp)
{
	printf("------------ DHCP ------------\n");
	printf("OP: %d\n", dhcp->opcode);
	printf("XID: 0x%X\n", ntohl(dhcp->xid));
	PrintDec("CIADDR: ", (unsigned char *)&dhcp->ciaddr, 4, "\n");
	PrintDec("YIADDR: ", (unsigned char *)&dhcp->yiaddr, 4, "\n");
	PrintDec("SIADDR: ", (unsigned char *)&dhcp->siaddr, 4, "\n");
	PrintDec("GIADDR: ", (unsigned char *)&dhcp->giaddr, 4, "\n");
	printf("---------- END DHCP ----------\n");
}

void PacketICMP(struct icmphdr *icmp)
{
	if (g_icmp)
	{
		printf("------------ ICMP ------------\n");
		printf("Type: %d\n", icmp->type);
		printf("Code: %d\n", icmp->code);
		printf("Checksum: %d\n", ntohs(icmp->checksum));
		printf("---------- END ICMP ----------\n");
	}
}

void PacketICMP6(struct icmp6_hdr *icmp)
{
	if (g_icmp6)
	{
		printf("----------- ICMP6 -----------\n");
		printf("Type: %d\n", icmp->icmp6_type);
		printf("Code: %d\n", icmp->icmp6_code);
		printf("Checksum: %d\n", ntohs(icmp->icmp6_cksum));
		printf("--------- END ICMP6 ---------\n");
	}
}

void PacketIP4(struct iphdr *ip)
{
	if (g_ip4)
	{
		printf("------------ IP4 ------------\n");
		printf("TTL: %d\n", ip->ttl);
		printf("Protocol: %d\n", ip->protocol);
		printf("Checksum: %d\n", ntohs(ip->check));
		PrintDec("Src IP: ", (unsigned char *)&ip->saddr, 4, "\n");
		PrintDec("Dst IP: ", (unsigned char *)&ip->daddr, 4, "\n");
	}

	switch (ip->protocol)
	{
	case IPPROTO_TCP:
		PacketTCP((struct tcphdr *)(ip + 1));
		break;
	case IPPROTO_UDP:
		PacketUPD((struct udphdr *)(ip + 1));
		break;
	case IPPROTO_ICMP:
		PacketICMP((struct icmphdr *)(ip + 1));
		break;
	}

	if (g_ip4)
	{
		printf("---------- END IP4 ----------\n");
	}
}

void PacketIP6(struct ip6_hdr *ip)
{
	if (g_ip6)
	{
		printf("------------ IP6 ------------\n");
		printf("Length: %d\n", ntohs(ip->ip6_plen));
		printf("Next header: %d\n", ip->ip6_nxt);
		printf("Hop limit: %d\n", ip->ip6_hlim);
		PrintDec("Src IP: ", (unsigned char *)&ip->ip6_src, 16, "\n");
		PrintDec("Dst IP: ", (unsigned char *)&ip->ip6_src, 16, "\n");
	}

	switch (ip->ip6_nxt)
	{
	case 58:
		PacketICMP6((struct icmp6_hdr *)(ip + 1));
		break;
	}

	if (g_ip6)
	{
		printf("---------- END IP6 ----------\n");
	}
}

void PacketTCP(struct tcphdr *tcp)
{
	if (g_tcp)
	{
		printf("------------ TCP ------------\n");
		printf("Src port: %d\n", ntohs(tcp->source));
		printf("Dst port: %d\n", ntohs(tcp->dest));
		printf("Seq: %d\n", ntohl(tcp->seq));
		printf("Ack: %d\n", ntohl(tcp->ack_seq));
		printf("Flags:\n");
		printf("  fin: %d\n", tcp->fin);
		printf("  syn: %d\n", tcp->syn);
		printf("  rst: %d\n", tcp->rst);
		printf("  psh: %d\n", tcp->psh);
		printf("  ack: %d\n", tcp->ack);
		printf("  urg: %d\n", tcp->urg);
		printf("Window: %d\n", ntohs(tcp->window));
		printf("Checksum: %d\n", ntohs(tcp->check));
		printf("Urgent pointer: %d\n", ntohs(tcp->urg_ptr));
		printf("---------- END TCP ----------\n");
	}
}

void PacketUPD(struct udphdr *udp)
{
	if (g_udp)
	{
		printf("------------ UDP ------------\n");
		printf("Src port: %d\n", ntohs(udp->source));
		printf("Dst port: %d\n", ntohs(udp->dest));
		printf("Length: %d\n", ntohs(udp->len));
		printf("Checksum: %d\n", ntohs(udp->check));
	}

	if ((ntohs(udp->source) == 67 || ntohs(udp->source) == 68) && (ntohs(udp->dest) == 67 || ntohs(udp->dest) == 68))
	{
		PacketDHCP((struct dhcphdr *)(udp + 1));
	}

	if (g_udp)
	{
		printf("---------- END UDP ----------\n");
	}
}

void ParseArgs(int argc, char **argv)
{
	if (argc == 2)
	{
		g_arp = 1;
		g_dhcp = 1;
		g_eth = 1;
		g_icmp = 1;
		g_icmp6 = 1;
		g_ip4 = 1;
		g_ip6 = 1;
		g_tcp = 1;
		g_udp = 1;
	}
	else
	{
		int option;
		
		opterr = 0;
		while ((option = getopt(argc, argv, "adeiI46tu")) != -1)
		{
			switch (option)
			{
			case 'a':
				g_arp = 1;
				break;
			case 'd':
				g_dhcp = 1;
				break;
			case 'e':
				g_eth = 1;
				break;
			case 'i':
				g_icmp = 1;
				break;
			case 'I':
				g_icmp6 = 1;
				break;
			case '4':
				g_ip4 = 1;
				break;
			case '6':
				g_ip6 = 1;
				break;
			case 't':
				g_tcp = 1;
				break;
			case 'u':
				g_udp = 1;
				break;
			}
		}
	}
}

void PrintHex(char *begin, unsigned char *hex, int size, char *end)
{
	printf("%s", begin);

	for (int i = 0; i < size; i++)
	{
		if (i == 0)
		{
			printf("%02x", hex[i]);
		}
		else
		{
			printf(":%02x", hex[i]);
		}
	}

	printf("%s", end);
}

void PrintDec(char *begin, unsigned char *dec, int size, char *end)
{
	printf("%s", begin);

	for (int i = 0; i < size; i++)
	{
		if (i == 0)
		{
			printf("%d", dec[i]);
		}
		else
		{
			printf(".%d", dec[i]);
		}
	}

	printf("%s", end);
}

void PrintError(char *function)
{
	char buffer[256];
	snprintf(buffer, 256, "Error %s", function);
	perror(buffer);
	exit(0);
}
