#ifndef __ARP_HELPER_H
#define __ARP_HELPER_H
// > Includes
#include "ConsoleLogger.h"
#include <string>
#include <vector>

#pragma region ARP Helpers / Function Declarations
// Name:  arpData
// Desc:  This is used to hold victim's information & attack data
struct arpData
{
	int		id;
	u_long	ip;
	u_char	mac[ETH_ALEN];
	u_char	arpServReq[sizeof(struct ethhdr) + sizeof(arphdr_ether)];			// < [UNUSED]
	u_char	arpServReply[sizeof(struct ethhdr) + sizeof(arphdr_ether)];			// < [UNUSED]
	u_char	arpClientReq[sizeof(struct ethhdr) + sizeof(arphdr_ether)];			// < [USED -> ARP POISON VICTIM]
	u_char	arpClientReply[sizeof(struct ethhdr) + sizeof(arphdr_ether)];		// < [UNUSED]
};

// > Function Declarations
int get_remote_mac(pcap_t *cap_dev, const u_char *if_addr, u_long sip, u_long dip, u_char *remote_mac);
void generate_arp_request(u_char *packet, const u_char *if_addr, u_long sip, u_long dip);
void generate_arp_reply(u_char *packet, const u_char *if_addr, const u_char *dst_haddr, u_long sip, u_long dip);
int process_arp_reply(struct pcap_pkthdr *header, const u_char *pkt_data, u_long sip, u_long dip, u_char *mac_result);
void packet_handler(u_char *param, const struct pcap_pkthdr *header, const u_char *pkt_data);
void dataSpammer();
#pragma endregion

#pragma region Structures
// Name:  ip_address
// Desc:  This is used to hold 4 bytes/segments of an IP address
//		  (Sourced from WinPCAP)
typedef struct ip_address {
	u_char byte1;
	u_char byte2;
	u_char byte3;
	u_char byte4;
} ip_address;

// Name:  ip_header
// Desc:  This is used to hold an IPv4 Header
//		  (Sourced from WinPCAP)
typedef struct ip_header {
	u_char  ver_ihl;			// Version (4 bits) + Internet header length (4 bits)
	u_char  tos;				// Type of service 
	u_short tlen;				// Total length 
	u_short identification;		// Identification
	u_short flags_fo;			// Flags (3 bits) + Fragment offset (13 bits)
	u_char  ttl;				// Time to live
	u_char  proto;				// Protocol
	u_short crc;				// Header checksum
	ip_address  saddr;			// Source address
	ip_address  daddr;			// Destination address
	u_int   op_pad;				// Option + Padding
} ip_header;

// Name:  udp_header
// Desc:  This is used to hold a UDP Header
//		  (Sourced from WinPCAP)
typedef struct udp_header {
	u_short sport;				// Source port
	u_short dport;				// Destination port
	u_short len;				// Datagram length
	u_short crc;				// Checksum
} udp_header;
#pragma endregion

#pragma region Raw Packet Helpers (Class)
// Name:  RawPacket
// Desc:  This class is used to generate raw UDP packets on the fly from given parameters.
class RawPacket
{
	public:
		unsigned char* FinalPacket;
		void CreatePacket(unsigned char* SourceMAC,
			unsigned char* DestinationMAC,
			unsigned int   SourceIP,
			unsigned int   DestinationIP,
			unsigned short SourcePort,
			unsigned short DestinationPort,
			unsigned char* UserData,
			unsigned int   DataLen);
		unsigned int UserDataLen;
		void SendPacket(pcap_t* handle);
	private:
		unsigned short CalculateUDPChecksum(unsigned char* UserData, int UserDataLen, UINT SourceIP, UINT DestIP, USHORT SourcePort, USHORT DestinationPort, UCHAR Protocol);
		unsigned short CalculateIPChecksum(UINT TotalLen, UINT ID, UINT SourceIP, UINT DestIP);
};
#pragma endregion

#pragma region Dummy Bots (Class)
// > This struct is only used by our dummy bots
struct Dummy {
	char *name,
		 *fname,
		 *lname;
	int   x,
		  y,
		  health,
		  flag,
		  type;
	bool  active;
};

class Bot
{
	private:
		// > Variables
		pcap_t*			handler;
		u_char			SERV_MAC[6];
		u_long			SERV_IP;
		CConsoleLoggerEx	console;

		// > Bot Data
		Dummy			me;

		// > Movement
		int				up_down;
		int				left_right;
		int				moveX;
		int				moveY;
		bool			moveShip;

		// > Flags
		bool			setFlag;
	public:
		// > Functions
		void Create(char* name, char* first_name, char* last_name, int type);
		void Listen();
		void MoveToPos(int x, int y);
		void MoveInDirection(int x, int y);
		void SetHandler(pcap_t* handle);
		void SetConsole(CConsoleLoggerEx cnsl);
		void SetServerData(u_long ip, u_char mac[6]);

		// > Variables
		u_char MAC[6];
		u_long IPv4;
};

#pragma endregion

#endif