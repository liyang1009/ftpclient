#pragma once
#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <fstream> 
#include <vector>
#pragma comment(lib, "Ws2_32.lib")

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")
using namespace std;
#define CRLF "\r\n"
//1xx
#define INITIATING 100
#define RESTART_MARKER 110
#define READY_MINUTE 120
#define ALREADY_OPEN 125
#define ABOUT_TO_SEND 150
//2xx
#define COMMAND_OK 200
#define COMMAND_NOT_IMPLEMENTED 202
#define SYSTEM 211
#define DIRECTORY 212
#define FILE 213
#define HELP 214
#define NAME 215
#define READY 220
#define CLOSING  221
#define DATA_CONNECTION_OPEN 225
#define CLOSING_DATA_CONNECTION 226
#define PASSIVE_MODE 227
#define LONG_PASSIVE_MODE 228
#define EXTENDED_PASSIVE_MODE 229
#define LOGGED_IN 230
#define LOGGED_OUT 231
#define LOGOUT_ACK 232
#define AUTH_OK 234
#define REQUESTED_FILE_ACTION_OK 250
#define PATH_CREATED 257
//3xx

#define NEED_PASSWORD 331
#define LOGIN_NEED_ACCOUNT 332
#define REQUEST_FILE_PENDING 350
//4xx
#define NOT_AVAILABLE 421
#define CANNOT_OPEN_DATA_CONNECTION 425
#define TRANSER_ABORTED 426
#define INVALID_CREDENTIALS 430
#define HOST_UNAVAILABLE 434
#define REQUEST_FILE_ACTION_IGNORED 450
#define	ACTION_ABORTED 451
#define	REQUESTED_ACTION_NOT_TAKEN 452

//5xx
#define BAD_COMMAND 500
#define	BAD_ARGUMENTS 501
#define NOT_IMPLEMENTED 502
#define BAD_SEQUENCE 503
#define NOT_IMPLEMENTED_PARAMETER 504
#define NOT_LOGGED_IN 530
#define STORING_NEED_ACCOUNT 532
#define FILE_UNAVAILABLE 550
#define PAGE_TYPE_UNKNOWN 551
#define	EXCEEDED_STORAGE 552
#define BAD_FILENAME 553


typedef struct line {
	int code;
	string line;
} line;
typedef struct address{
	string ip;
	int port;
} address;
class ftp
{
public:
	ftp();
	~ftp();
	int fd;
	SOCKET conn_socket;
	bool isconnected;
	bool connection(string host, int port);
	void login(string username, string password);
	void sendCommand(string command);
	void cwd(string path);
	string pwd();
	void putLine(string command);
	void put(string filepath);
	string  readLine();
	void simple_retry(string r_file,string l_file);
	string list(string path);

private:
	vector<string>   getVaildAddress(string& str);
	SOCKET data_command(string command);
	line * readLineIn(int * codes, int len);
	line * readLineInCode(int code);
	address  pasv();
};


