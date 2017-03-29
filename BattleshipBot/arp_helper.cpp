/**
* -------------------------------
* |     Author  Information     |
* -------------------------------
* Reece Benson - 16021424
* BSc Computer Science - Year 1
* Battleship Bots
**/
#pragma region Includes & Definitions
// > Includes
#include "stdafx.h"
#include <thread>
using namespace std;
#include "pcap.h"
#include "ether.h"
#include "arp.h"
#include "arp_helper.h"
// > Definitions
#define CAP_MAX_TIMEOUTS 50
#define CAP_MAX_PACKETS	50
#pragma endregion

#pragma region ARP Helpers
// Name:  get_remote_mac(pcap_t *cap_dev, const u_char *if_addr, u_long sip, u_long dip, u_char *remote_mac);
// Args:  5
// Desc:  This will return a status response to see if we have received the MAC address of the address specified.
//		  The MAC Address is stored into the specified pointer variable (in this case, *remote_mac)
int get_remote_mac(pcap_t *cap_dev, const u_char *if_addr, u_long sip, u_long dip, u_char *remote_mac) {
	
	u_char arp_packet[sizeof(struct ethhdr) + sizeof(arphdr_ether)];
	struct pcap_pkthdr *header;
	const u_char *pkt_data;
	int res = 0;

	/* In case we fail */
	memset(remote_mac, 0, ETH_ALEN);

	/* Build the ARP request packet */
	generate_arp_request(arp_packet, if_addr, sip, dip);
	/* Send the ARP request */
	/* There's a race condition here because the reply might arrive before we capture it */
	if (pcap_sendpacket(cap_dev, arp_packet, sizeof(arp_packet)) != 0)
		return -1;
	int timeouts = 0;
	int packets = 0;
	/* Start the capture */
 	while ((res = pcap_next_ex(cap_dev, &header, &pkt_data)) >= 0)
	{

		//static int timeouts = 0;
		//static int packets = 0;

		/* Timeout elapsed? */
		if (res == 0) {
			if (++timeouts > CAP_MAX_TIMEOUTS)
				break;
		}
		else {
			/* Look for the ARP reply (source and destination are reversed) */
			if (process_arp_reply(header, pkt_data, dip, sip, remote_mac) == 0)
			{
				//printf("t:%d, p:%d\t",timeouts,packets);
				return 0;
			}
			/* Seen too many packets without finding ours? */
			if (++packets >= CAP_MAX_PACKETS)
						break;
		}
	}
	/* Didn't receive the appropriate ARP reply */
	return -2;
}

// Name:  generate_arp_request(u_char *packet, const u_char *if_addr, u_long sip, u_long dip);
// Args:  4
// Desc:  This will generate an ARP request (setup an ARP Poisoned Cache) to the destination IP Address.
//		  The request is generated from the given parameters.
void generate_arp_request(u_char *packet, const u_char *if_addr, u_long sip, u_long dip) {

	struct ethhdr *ethp;
	struct arphdr_ether *arpp;

	/* Fill Ethernet data */
	ethp = (struct ethhdr *)packet;
	ethp->h_proto = htons(ETHERTYPE_ARP);
	memcpy(ethp->h_source, if_addr, ETH_ALEN);
	/* Send to the broadcast address */
	memset(ethp->h_dest, 0xFF, ETH_ALEN);

	/* Fill ARP data */
	arpp = (struct arphdr_ether *)(packet + sizeof(struct ethhdr));
	arpp->ar_hrd = htons(ARPHRD_ETHER);
	arpp->ar_pro = htons(ETHERTYPE_IP);
	arpp->ar_hln = ETH_ALEN;
	arpp->ar_pln = sizeof(u_long);
	arpp->ar_op = htons(ARPOP_REQUEST);
	memcpy(arpp->ar_sha, if_addr, ETH_ALEN);
	memcpy(arpp->ar_sip, &sip, sizeof(u_long));
	/* We don't know who the target is */
	memset(arpp->ar_tha, 0, ETH_ALEN);
	memcpy(arpp->ar_tip, &dip, sizeof(u_long));
}

// Name:  generate_arp_reply(u_char *packet, const u_char *if_addr, const u_char *dst_haddr, u_long sip, u_long dip);
// Args:  5
// Desc:  This will generate an ARP reply (to reply back to the poisoned victim) to the destination IP Address.
//		  The reply is generated from the given parameters.
void generate_arp_reply(u_char *packet, const u_char *if_addr, const u_char *dst_haddr, u_long sip, u_long dip) {

	struct ethhdr *ethp;
	struct arphdr_ether *arpp;

	/* Fill Ethernet data */
	ethp = (struct ethhdr *)packet;
	ethp->h_proto = htons(ETHERTYPE_ARP);
	memcpy(ethp->h_source, if_addr, ETH_ALEN);
	/* Send to the destination address */
	memcpy(ethp->h_dest, dst_haddr, ETH_ALEN);

	/* Fill ARP data */
	arpp = (struct arphdr_ether *)(packet + sizeof(struct ethhdr));
	arpp->ar_hrd = htons(ARPHRD_ETHER);
	arpp->ar_pro = htons(ETHERTYPE_IP);
	arpp->ar_hln = ETH_ALEN;
	arpp->ar_pln = sizeof(u_long);
	arpp->ar_op = htons(ARPOP_REPLY);
	memcpy(arpp->ar_sha, if_addr, ETH_ALEN);
	memcpy(arpp->ar_sip, &sip, sizeof(u_long));
	memcpy(arpp->ar_tha, dst_haddr, ETH_ALEN);
	memcpy(arpp->ar_tip, &dip, sizeof(u_long));
}

// Name:  process_arp_reply(struct pcap_pkthdr *header, const u_char *pkt_data, u_long sip, u_long dip, u_char *remote_mac);
// Args:  5
// Desc:  This will process the reply (used in get_remote_mac) received by the source and will check that the victim
//		  has been poisoned correctly. This will return 0 for success, and any other value below 0 for failures.
int process_arp_reply(struct pcap_pkthdr *header, const u_char *pkt_data, u_long sip, u_long dip, u_char *remote_mac) {

	const struct ethhdr *ethp;
	const struct arphdr_ether *arpp;

	/* Sanity checks */
	if (header->caplen < sizeof(struct ethhdr) + sizeof(arphdr_ether))
		/* Packet too small */
		return -1;

	ethp = (struct ethhdr *)pkt_data;
	if (ethp->h_proto != htons(ETHERTYPE_ARP))
		/* Not an ARP packet */
		return -2;

	arpp = (struct arphdr_ether *)(pkt_data + sizeof(struct ethhdr));
	if (arpp->ar_hrd != htons(ARPHRD_ETHER))
		/* ARP not for Ethernet */
		return -3;
	if (arpp->ar_pro != htons(ETHERTYPE_IP))
		/* ARP not for IP */
		return -4;

	if (ntohs(arpp->ar_op) != ARPOP_REPLY)
		/* Not an ARP reply */
		return -5;

	if (*((u_long *)&(arpp->ar_sip)) != sip)
		/* Not the source we were looking for */
		return -6;

	if (*((u_long *)&(arpp->ar_tip)) != dip)
		/* Not the destination we were looking for */
		return -7;

	/* Everything looks ok - copy the ethernet address of the sender */
	memcpy(remote_mac, arpp->ar_sha, ETH_ALEN);
	return 0;
}
#pragma endregion

#pragma region Byte Conversion / String Manipulation
// Name:  BytesTo16(unsigned char X, unsigned char Y);
// Args:  2
// Desc:  This will convert 16 bytes into ushort
unsigned short BytesTo16(unsigned char X, unsigned char Y)
{
	unsigned short Tmp = X;
	Tmp = Tmp << 8;
	Tmp = Tmp | Y;
	return Tmp;
}

// Name:  BytesTo32(unsigned char W, unsigned char X, unsigned char Y, unsigned char Z);
// Args:  4
// Desc:  This will convert 32 bytes into uint
unsigned int BytesTo32(unsigned char W, unsigned char X, unsigned char Y, unsigned char Z)
{
	unsigned int Tmp = W;
	Tmp = Tmp << 8;
	Tmp = Tmp | X;
	Tmp = Tmp << 8;
	Tmp = Tmp | Y;
	Tmp = Tmp << 8;
	Tmp = Tmp | Z;
	return Tmp;
}

// Name:  split(const string& str, const string& delim);
// Args:  2
// Desc:  Splits a string.
//		  [SOURCE] = http://stackoverflow.com/a/37454181
vector<string> split(const string& str, const string& delim)
{
	vector<string> tokens;
	size_t prev = 0, pos = 0;
	do
	{
		pos = str.find(delim, prev);
		if (pos == string::npos) pos = str.length();
		string token = str.substr(prev, pos - prev);
		if (!token.empty()) tokens.push_back(token);
		prev = pos + delim.length();
	} while (pos < str.length() && prev < str.length());
	return tokens;
}
#pragma endregion

#pragma region Raw Packet Helpers
// Name:  CreatePacket(unsigned char* SourceMAC, unsigned char* DestinationMAC, unsigned int SourceIP, unsigned int DestIP, unsigned short SourcePort, unsigned short DestinationPort, unsigned char* UserData, unsigned int UserDataLen);
// Class: RawPacket
// Args:  8
// Desc:  This will create a UDP Packet from scratch from the given parameters.
void RawPacket::CreatePacket(unsigned char* SourceMAC, unsigned char* DestinationMAC, unsigned int SourceIP, unsigned int DestIP, unsigned short SourcePort, unsigned short DestinationPort, unsigned char* UserData, unsigned int UserDataLen)
{
	RawPacket::UserDataLen = UserDataLen;
	FinalPacket = new unsigned char[UserDataLen + 42]; // Reserve enough memory for the length of the data plus 42 bytes of headers 
	USHORT TotalLen = UserDataLen + 20 + 8; // IP Header uses length of data plus length of ip header (usually 20 bytes) plus lenght of udp header (usually 8)
											//Beginning of Ethernet II Header
	memcpy((void*)FinalPacket, (void*)DestinationMAC, 6);
	memcpy((void*)(FinalPacket + 6), (void*)SourceMAC, 6);
	USHORT TmpType = 8;
	memcpy((void*)(FinalPacket + 12), (void*)&TmpType, 2); //The type of protocol used. (USHORT) Type 0x08 is UDP. You can change this for other protocols (e.g. TCP)
														   // Beginning of IP Header
	memcpy((void*)(FinalPacket + 14), (void*)"\x45", 1); //The Version (4) in the first 3 bits  and the header length on the last 5. (Im not sure, if someone could correct me plz do)
														 //If you wanna do any IPv6 stuff, you will need to change this. but i still don't know how to do ipv6 myself =s 
	memcpy((void*)(FinalPacket + 15), (void*)"\x00", 1); //Differntiated services field. Usually 0 
	TmpType = htons(TotalLen);
	memcpy((void*)(FinalPacket + 16), (void*)&TmpType, 2);
	TmpType = htons(0x1337);
	memcpy((void*)(FinalPacket + 18), (void*)&TmpType, 2);// Identification. Usually not needed to be anything specific, esp in udp. 2 bytes (Here it is 0x1337
	memcpy((void*)(FinalPacket + 20), (void*)"\x00", 1); // Flags. These are not usually used in UDP either, more used in TCP for fragmentation and syn acks i think 
	memcpy((void*)(FinalPacket + 21), (void*)"\x00", 1); // Offset
	memcpy((void*)(FinalPacket + 22), (void*)"\x80", 1); // Time to live. Determines the amount of time the packet can spend trying to get to the other computer. (I see 128 used often for this)
	memcpy((void*)(FinalPacket + 23), (void*)"\x11", 1);// Protocol. UDP is 0x11 (17) TCP is 6 ICMP is 1 etc
	memcpy((void*)(FinalPacket + 24), (void*)"\x00\x00", 2); //checksum 
	memcpy((void*)(FinalPacket + 26), (void*)&SourceIP, 4); //inet_addr does htonl() for us
	memcpy((void*)(FinalPacket + 30), (void*)&DestIP, 4);
	//Beginning of UDP Header
	TmpType = htons(SourcePort);
	memcpy((void*)(FinalPacket + 34), (void*)&TmpType, 2);
	TmpType = htons(DestinationPort);
	memcpy((void*)(FinalPacket + 36), (void*)&TmpType, 2);
	USHORT UDPTotalLen = htons(UserDataLen + 8); // UDP Length does not include length of IP header
	memcpy((void*)(FinalPacket + 38), (void*)&UDPTotalLen, 2);
	//memcpy((void*)(FinalPacket+40),(void*)&TmpType,2); //checksum
	memcpy((void*)(FinalPacket + 42), (void*)UserData, UserDataLen);

	unsigned short UDPChecksum = CalculateUDPChecksum(UserData, UserDataLen, SourceIP, DestIP, htons(SourcePort), htons(DestinationPort), 0x11);
	memcpy((void*)(FinalPacket + 40), (void*)&UDPChecksum, 2);

	unsigned short IPChecksum = htons(CalculateIPChecksum(TotalLen, 0x1337, SourceIP, DestIP));
	memcpy((void*)(FinalPacket + 24), (void*)&IPChecksum, 2);

	return;

}

// Name:  CalculateIPChecksum(UINT TotalLen, UINT ID, UINT SourceIP, UINT DestIP);
// Class: RawPacket
// Args:  4
// Desc:  This will calculate and return the IP Checksum
unsigned short RawPacket::CalculateIPChecksum(UINT TotalLen, UINT ID, UINT SourceIP, UINT DestIP)
{
	unsigned short CheckSum = 0;
	for (int i = 14; i<34; i += 2)
	{
		unsigned short Tmp = BytesTo16(FinalPacket[i], FinalPacket[i + 1]);
		unsigned short Difference = 65535 - CheckSum;
		CheckSum += Tmp;
		if (Tmp > Difference) { CheckSum += 1; }
	}
	CheckSum = ~CheckSum;
	return CheckSum;
}

// Name:  CalculateIPChecksumCalculateUDPChecksum(unsigned char* UserData, int UserDataLen, UINT SourceIP, UINT DestIP, USHORT SourcePort, USHORT DestinationPort, UCHAR Protocol);
// Class: RawPacket
// Args:  7
// Desc:  This will calculate and return the UDP Checksum
unsigned short RawPacket::CalculateUDPChecksum(unsigned char* UserData, int UserDataLen, UINT SourceIP, UINT DestIP, USHORT SourcePort, USHORT DestinationPort, UCHAR Protocol)
{
	unsigned short CheckSum = 0;
	unsigned short PseudoLength = UserDataLen + 8 + 9; //Length of PseudoHeader = Data Length + 8 bytes UDP header (2Bytes Length,2 Bytes Dst Port, 2 Bytes Src Port, 2 Bytes Checksum)
													   //+ Two 4 byte IP's + 1 byte protocol
	PseudoLength += PseudoLength % 2; //If bytes are not an even number, add an extra.
	unsigned short Length = UserDataLen + 8; // This is just UDP + Data length. needed for actual data in udp header

	unsigned char* PseudoHeader = new unsigned char[PseudoLength];
	for (int i = 0; i < PseudoLength; i++) { PseudoHeader[i] = 0x00; }

	PseudoHeader[0] = 0x11;

	memcpy((void*)(PseudoHeader + 1), (void*)(FinalPacket + 26), 8); // Source and Dest IP

	Length = htons(Length);
	memcpy((void*)(PseudoHeader + 9), (void*)&Length, 2);
	memcpy((void*)(PseudoHeader + 11), (void*)&Length, 2);

	memcpy((void*)(PseudoHeader + 13), (void*)(FinalPacket + 34), 2);
	memcpy((void*)(PseudoHeader + 15), (void*)(FinalPacket + 36), 2);

	memcpy((void*)(PseudoHeader + 17), (void*)UserData, UserDataLen);


	for (int i = 0; i < PseudoLength; i += 2)
	{
		unsigned short Tmp = BytesTo16(PseudoHeader[i], PseudoHeader[i + 1]);
		unsigned short Difference = 65535 - CheckSum;
		CheckSum += Tmp;
		if (Tmp > Difference) { CheckSum += 1; }
	}
	CheckSum = ~CheckSum; //One's complement
	return CheckSum;
}

// Name:  SendPacket(pcap_t* handle);
// Class: RawPacket
// Args:  1
// Desc:  This will send the generate packet (from CreatePacket(...)) to the specified handle.
void RawPacket::SendPacket(pcap_t* handle)
{
	if (pcap_sendpacket(handle, FinalPacket, UserDataLen + 42) != 0)
	{
		fprintf(stderr, "\nError sending the packet: \n", pcap_geterr(handle));

		// > Handle Error
		/*printf("Trying to send packet:\n");
		for (int i = 0; i < (UserDataLen + 42); i++)
		{
			printf("%.2x", FinalPacket[i]);

			if ((i % 16) == 0 && i != 0)
				printf("\n");
		}
		printf("error help\n");*/
	}
	
	// > If the packet doesn't send, don't worry about it, they'll just freeze instead
}
#pragma endregion

#pragma region Dummy Bots
// Name:  Create(char* name, char* first_name, char* last_name, int type);
// Class: Bot
// Args:  4
// Desc:  Initialises a new bot into the server from the given parameters.
void Bot::Create(char* name, char* first_name, char* last_name, int type)
{
	// > Create our initial buffer in 'char' array
	char regBuffT[256];
	sprintf_s(regBuffT, "Register  %s,%s,%s,%d", name, first_name, last_name, type);

	// > Recreate our buffer with 'u_char' array
	u_char regBuff[256];
	int    regBuffLen;
	for (int i = 0; i < 256; i++)
	{
		// > Add to our 'u_char' buffer
		regBuff[i] = regBuffT[i];

		// > Have we reached our terminator?
		if (regBuff[i] == '\0')
		{
			regBuffLen = i;
			break;
		}
	}

	// > Update our bot's data
	this->me.name = name;
	this->me.fname = first_name;
	this->me.lname = last_name;
	this->me.x = 0;
	this->me.y = 0;
	this->me.health = 10;
	this->me.flag = 133337;
	this->me.type = type;
	this->me.active = true;
	this->setFlag = true;
	this->moveShip = true;
	this->left_right = (-1 * 2);	// MOVE_LEFT*MOVE_FAST
	this->up_down = (1 * 2);		// MOVE_UP*MOVE_FAST

	// > Send our command with a fake raw packet to register our Bot
	RawPacket p;
	p.CreatePacket(this->MAC, this->SERV_MAC, this->IPv4, this->SERV_IP, 60666, 1924, regBuff, regBuffLen);
	p.SendPacket(this->handler);

	// > Debug
	//this->console.printf("[BOT] -> Created bot: [%s] (%s %s) -> %d\n", name, first_name, last_name, type);
}

// Name:  Listen();
// Class: Bot
// Args:  N/A
// Desc:  Start listening to the bot made from the generated IP/MAC Address
void Bot::Listen()
{
	while (this->me.active)
	{
		// > Define our packet
		RawPacket p;

		// [TODO] > Implement it so we can see where our dummy ship is
		if (this->setFlag)
		{
			// > Set our bots flag
			// > Create our initial buffer in 'char' array
			char flagBuffT[256];
			sprintf_s(flagBuffT, "Flag %s,%d", this->me.name, this->me.flag);

			// > Recreate our buffer with 'u_char' array
			u_char flagBuff[256];
			int	   flagBuffLen;
			for (int i = 0; i < 256; i++)
			{
				// > Add to our 'u_char' buffer
				flagBuff[i] = flagBuffT[i];

				// > Have we reached our terminator?
				if (flagBuff[i] == '\0')
				{
					flagBuffLen = i;
					break;
				}
			}

			// > Send our command with a fake raw packet to set our flag of our bot
			p.CreatePacket(this->MAC, this->SERV_MAC, this->IPv4, this->SERV_IP, 60666, 1924, flagBuff, flagBuffLen);
			p.SendPacket(this->handler);

			// > Reset our flag
			this->setFlag = false;
		}

		if (this->moveShip)
		{
			// > Let's move our ship to the bottom left for now
			// -> Create our initial buffer in 'char' array
			char buffT[256];
			sprintf_s(buffT, "Move %s,%d,%d", this->me.name, -2, -2);

			// -> Recreate our buffer with 'u_char' array
			u_char buff[256];
			int    buffLen;
			for (int i = 0; i < 256; i++)
			{
				// > Add to our 'u_char' buffer
				buff[i] = buffT[i];

				// > Have we reached our terminator?
				if (buff[i] == '\0')
				{
					buffLen = i;
					break;
				}
			}

			// > Send our move command
			p.CreatePacket(this->MAC, this->SERV_MAC, this->IPv4, this->SERV_IP, 60666, 1924, buff, buffLen);
			p.SendPacket(this->handler);
		}

		// > This is the sleep time of the server, we don't want to send hundreds of useless packets so sleep for 100ms
		Sleep(100);
	}
}

// Name:  MoveToPos(int x, int y);
// Class: Bot
// Args:  2
// Desc:  Starts moving the bot to the specified X and Y coordinates.
void Bot::MoveToPos(int x, int y)
{
	// > Movements (do not redefine)
	int MOVE_LEFT = -1;
	int MOVE_RIGHT = 1;
	int MOVE_UP = 1;
	int MOVE_DOWN = -1;
	int MOVE_FAST = 2;
	int MOVE_SLOW = 1;

	// > Default Values
	int moveSpeedX = MOVE_FAST;
	int moveSpeedY = MOVE_FAST;

	// > Get differences between our ship and the new pos
	int diffX = this->me.x - x;
	int diffY = this->me.y - y;

	// > If we're close / ontop of the enemy ship, move slowly
	if (abs(diffX) == 1) moveSpeedX = MOVE_SLOW;
	if (abs(diffY) == 1) moveSpeedY = MOVE_SLOW;

	// > Set our X movement
	if (diffX > 0)		this->left_right = MOVE_LEFT  * moveSpeedX;
	else if (diffX < 0) this->left_right = MOVE_RIGHT * moveSpeedX;
	else				this->left_right = 0;

	// > Set our Y movement
	if (diffY > 0)		this->up_down = MOVE_DOWN * moveSpeedY;
	else if (diffY < 0) this->up_down = MOVE_UP * moveSpeedY;
	else				this->up_down = 2;

	// > Let's move!
	this->MoveInDirection(this->left_right, this->up_down);
}

// Name:  crawlInDirection(int x, int y);
// Args:  2
// Desc:  Move our bot in a specified direction.
void Bot::MoveInDirection(int x, int y)
{
	if (x < -2) x = -2;
	if (x >  2) x = 2;
	if (y < -2) y = -2;
	if (y >  2) y = 2;

	// > Set our moving ship flag
	moveShip = true;
	this->moveX = x;
	this->moveY = y;
}

// Name:  SetHandle(pcap_t* hndl);
// Class: Bot
// Args:  1
// Desc:  Set the handler for the bot (this will be used to send packets)
void Bot::SetHandler(pcap_t* handle)
{
	// > Set our handler
	this->handler = handle;
}

// Name:  SetConsole(CConsoleLoggerEx cnsl);
// Class: Bot
// Args:  1
// Desc:  Set the handler for the bot (this will be used to send packets)
void Bot::SetConsole(CConsoleLoggerEx cnsl)
{
	// > Set our handler
	this->console = cnsl;
}

// Name:  SetServerData(u_long ip, u_char mac[6]);
// Class: Bot
// Args:  2
// Desc:  Set the IP and MAC address of the server so that the Bot knows where to send data
void Bot::SetServerData(u_long ip, u_char mac[6])
{
	// > Set our IP
	this->SERV_IP = ip;

	// > Set our MAC
	for (int i = 0; i < 5; i++)
		this->SERV_MAC[i] = mac[i];
}
#pragma endregion