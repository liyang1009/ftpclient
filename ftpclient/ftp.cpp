#include "pch.h"
#include "ftp.h"
#pragma warning(disable:4996) 
ftp::ftp()
{
	this->conn_socket = INVALID_SOCKET;
}
ftp::~ftp()
{
}

std::vector<std::string> split(const std::string &text, char sep) {
	std::vector<std::string> tokens;
	std::size_t start = 0, end = 0;
	while ((end = text.find(sep, start)) != std::string::npos) {
		tokens.push_back(text.substr(start, end - start));
		start = end + 1;
	}
	tokens.push_back(text.substr(start));
	return tokens;
}
//get the communication socket
SOCKET getSocket(address ip_port) {
	sockaddr_in clientService;
	clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = inet_addr(ip_port.ip.c_str());
	clientService.sin_port = htons(ip_port.port);
	// Create a SOCKET for connecting to server
	SOCKET conn_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (conn_socket == INVALID_SOCKET) {
		wprintf(L"socket function failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}
	int iResult = connect(conn_socket, (sockaddr *)&clientService, sizeof(clientService));
	if (iResult == SOCKET_ERROR) {
		wprintf(L"connect function failed with error: %ld\n", WSAGetLastError());
		iResult = closesocket(conn_socket);
		if (iResult == SOCKET_ERROR)
			wprintf(L"closesocket function failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}
	return conn_socket;
}
bool ftp::connection(string host, int port) {

	sockaddr_in clientService;
	clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = inet_addr(host.c_str());
	clientService.sin_port = htons(port);
	// Create a SOCKET for connecting to server
	this->conn_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (conn_socket == INVALID_SOCKET) {
		wprintf(L"socket function failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}
	int iResult = connect(this->conn_socket, (sockaddr *)&clientService, sizeof(clientService));
	if (iResult == SOCKET_ERROR) {
		wprintf(L"connect function failed with error: %ld\n", WSAGetLastError());
		iResult = closesocket(this->conn_socket);
		if (iResult == SOCKET_ERROR)
			wprintf(L"closesocket function failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}
	
	
	line * res_line = this->readLineInCode(READY);
	bool connect_state = false;
	if (!res_line) {
		connect_state = true;
	}
	delete res_line;
	return connect_state;
}

/*
 user login  by username and password
*/
void ftp::login(string username, string password) {

	bool result = true;
	char send_str[128] = { 0 };
	sprintf_s(send_str, "USER %s", username.c_str());
	string command = send_str;
	this->sendCommand(command);
	int codes[2] = { LOGGED_IN,NEED_PASSWORD };
	line * res_line = this->readLineIn(codes, 2);
	if (res_line) {
		if (res_line->code == NEED_PASSWORD) {
			memset(send_str, 0, 128);
			sprintf_s(send_str, "PASS %s", password.c_str());
			string password_command = send_str;
			this->sendCommand(password_command);
			line * line_login = this->readLineInCode(LOGGED_IN);
			if (line_login) {

				delete line_login;
			}
			else {
				result = false;
			}
		}
		delete res_line;
	}
	if (!result) {
		throw exception();
	}

}

// send the cwd command to server
void ftp::cwd(string filepath) {
	char send_str[128] = { 0 };
	sprintf_s(send_str, "CWD %s", filepath.c_str());
	string command = send_str;
	this->sendCommand(command);
	line * res_line = this->readLineInCode(REQUESTED_FILE_ACTION_OK);
	if (res_line) {
		delete res_line;
	}
	else {
		throw exception();
	}

}
//implement the pwd command
string ftp::pwd() {

	string command  = "PWD";
	this->sendCommand(command);
	line * res_line = this->readLineInCode(PATH_CREATED);
	int left_index = res_line->line.find_first_of("\"");
	int right_index = res_line->line.find_last_of("\"");
	int count = res_line->line.length() - left_index - 1;
	string content = "";
	if (left_index < right_index) {
		content = res_line->line.substr(left_index + 1, count);
	}
	delete res_line;
	return content;
}

//set pass mode
address ftp::pasv() {
	string command = "PASV";
	address ip_port = { "",0 };
	this->sendCommand(command);
	line * res_line = this->readLineInCode(PASSIVE_MODE);
	if (res_line != NULL  ) {
		bool include_port = false;
		for (size_t i = 0; i < res_line->line.size(); ++i)
		{
			if (isdigit(res_line->line[i])) { 
				include_port = true;
				break;
			}
			
		}
		if (include_port) {
			vector<string>  vaild_ip_str = getVaildAddress(res_line->line);
			string oct1 = vaild_ip_str.at(0);
			string oct2 = vaild_ip_str[1];
			string oct3 = vaild_ip_str[2];
			string oct4 = vaild_ip_str[3];
			int msb, lsb;
			msb = atoi(vaild_ip_str.at(4).c_str());
			lsb = atoi(vaild_ip_str[5].c_str());
			short port = (msb << 8) | lsb;
			char ip[128] = { 0 };
			string ip_str;
			ip_str += oct1 +".";
			ip_str += oct2 + ".";
			ip_str += oct3 + ".";
			ip_str += oct4;
			ip_port.ip = ip_str;
			ip_port.port = port;

		}

	}
	delete res_line;
	return ip_port;
}

//get the receive data channel
SOCKET ftp::data_command(string command) {
	address  result = this->pasv();
	if (result.port) {
		this->sendCommand(command);

;		
		return getSocket(result);
	}
	return NULL;
}

//get the remote file 
void ftp::simple_retry(string r_file, string l_file) {

	char send_str[128] = { 0 };
	sprintf_s(send_str, "RETR %s", r_file.c_str());
	string command = send_str;
	SOCKET socket_ = this->data_command(command);
	

	int codes[2] = { ABOUT_TO_SEND ,ALREADY_OPEN };
	line * res_line = this->readLineIn(codes, 2);
	if (socket_) {
		char buf[4096] = { 0 };
		std::ofstream  file(l_file, std::ifstream::binary);
		int recv_len = 0;
		while ((recv_len = recv(socket_, buf, 4096, 0)) > 0) {
			if (file.is_open())
			{
				file.write(buf, recv_len);
			}

			memset(buf, 0, 4096);
		}
		file.close();

	}
	bool result = false;
	if (res_line) {
		delete res_line;
		int close_res = closesocket(socket_);
		res_line = NULL;
		codes[0] = CLOSING_DATA_CONNECTION;
		codes[1] = REQUESTED_FILE_ACTION_OK;
		res_line = this->readLineIn(codes, 2);
		if (res_line) {
			result = true;
		}
		delete res_line;
	}
	
	if (result) {
		return;
	}

}
string ftp::list(string path) {


	return "";
}

void ftp::put(string filename) {

	int offset = filename.find_last_of("\\");
	int count = filename.length() - offset;
	string fname = filename.substr(offset+1, count);
	char command[128] = { 0 };
	sprintf(command, "STOR %s", fname.c_str());
	string command_str = command;
	SOCKET _socket = this->data_command(command_str);
	int codes[2] = { ALREADY_OPEN ,ABOUT_TO_SEND };
	line * res_line = this->readLineIn(codes, 2);
	if (res_line&&_socket) {

		std::ifstream  file(filename, std::ifstream::binary);
		char buffer[4096] = { 0 };
		std::streamsize bytes = 0;
		if (file)
		{
			while (!file.eof()) {
				file.read(buffer, 4096);
				bytes = file.gcount();
				send(_socket, buffer, (int)bytes, 0);
				memset(buffer, 0, (int)bytes);
			}
			file.close();
		}
	
		closesocket(_socket);

		int w_codes[2] = { CLOSING_DATA_CONNECTION ,REQUESTED_FILE_ACTION_OK };
		line * res_line_conn = this->readLineIn(w_codes,2);
		if (res_line_conn) {
			delete res_line_conn;
		}
		

	}
	if (res_line) {
		delete res_line;
	}
	
	
}
/*
* read the response data include the CRLF
*/
string ftp::readLine() {

	string  str = "";
	char buf[1024] = { 0 };
	int exists_size = 0;
	int recv_code = 0;
	while ((recv_code = recv(this->conn_socket, buf, 1024, 0))> 0) {
		str += string(buf);
		exists_size = str.find(CRLF);
		if (exists_size>= 0) {
			break;
		}
	
		memset(buf, 0, 1024);
	}
	int error_code = WSAGetLastError();
	return str;
}
//read the command channel response line
line * ftp::readLineIn(int * codes,int len) {
	string str = this->readLine();

	int code = std::stoi(str.substr(0, 3));
	string expect_str = str.substr(0, 3);
	while (str.length() < 5 || str.substr(0, 3) != expect_str) {
		str.clear();
		str = this->readLine();

	}
	for (int index = 0; index < len; index++) {
		if (code == codes[index]) {
			line * res_line = new line();
			res_line->code = code;
			res_line->line = str;
			return res_line;
		}
	}
	return NULL;
}


/*
* read specify code line	
*/
line * ftp::readLineInCode(int code) {

	int codes[1] = {code};
	return this->readLineIn(codes,1);
}
void ftp::sendCommand(string command) {
	if (command.find("\n") > 0 || command.find("\r")) {
		//throw exception();
	}
	command += CRLF;
	send(this->conn_socket, command.c_str(), command.length(), 0);
}
void ftp::putLine(string command) {
	
}

vector<string>  ftp::getVaildAddress(string& str) {
	int start = str.find_first_of("(");
	int end = str.find_first_of(")");
	string need_str = str.substr(start+1, end - start-1);
	vector<string> split_one = ::split(need_str, ',');
	return split_one;
}