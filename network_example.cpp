#include <iostream>
#include <string>
#include <chrono> 
#include <thread>

#include <winsock2.h>
#include <ws2tcpip.h>

#include "utilities.h"

// If using MSVC compiler, this links your program automatically with winsock2.
// Otherwise it needs to be compiled with the flag -lws2_32
#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_PORT "27015"
#define LOCAL_IP     "127.0.0.1"

// Incase of confusion, addrinfo is used for two different things...


/* 
	UDP Example

	int udp_fd = ::socket(AF_INET, SOCK_DGRAM, 0);

    // Define destination address (e.g., 192.168.1.50:9000)
    sockaddr_in dest_addr{};
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(9000);
    ::inet_pton(AF_INET, "192.168.1.50", &dest_addr.sin_addr);

    const char* message = "Hello UDP!";

    // 2. Just send it!
    ::sendto(udp_fd, message, 10, 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));

*/




void runClient() {

	struct addrinfo settings;
	ZeroMemory(&settings, sizeof(settings));
	settings.ai_family   = AF_INET;
	settings.ai_socktype = SOCK_STREAM;
	settings.ai_protocol = IPPROTO_TCP;

	struct addrinfo* result;
	// Get network address AND information regarding it. Store in result(s).
	getaddrinfo(LOCAL_IP,     // IP
				DEFAULT_PORT, // PORT
				&settings,    //
				&result);

	SOCKET connect_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	int err = connect(connect_socket, result->ai_addr, (int)result->ai_addrlen);


	const char *sendbuf = "hello";
	send(connect_socket, sendbuf, 6, 0);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

}


void runServer() {

	struct addrinfo settings;
	ZeroMemory(&settings, sizeof(settings));
	settings.ai_family   = AF_INET; // Adress family. INET is Ipv4, INET6 is ipv6. UNSPEC is whatever.
	settings.ai_socktype = SOCK_STREAM;
	settings.ai_protocol = IPPROTO_TCP;
	settings.ai_flags    = AI_PASSIVE;

	struct addrinfo* result;
	// Get network address AND information regarding it. Store in result(s).
	getaddrinfo(NULL,         // IP
				DEFAULT_PORT, // PORT
				&settings,    // Data IN
				&result);     // Results OUT. Contains both 'settings' & ip/port formatted for the computer.

	// Create socket. Initializes a socket with given settings. Still not active in any way.
	SOCKET listen_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	// Bind socket. Connects the socket to our created network address. Since we are using TCP, it is still not connected to anything, thus it cannot send data.
	bind(listen_socket, result->ai_addr, (int)result->ai_addrlen);

	// Setup socket a listen socket. Allows for 10 attempted connections tp pile up before new ones being discarded.
	listen(listen_socket, 10);

	utilities::print_line("waiting for client...");
	// NULLs are client addr & len.
	SOCKET client_socket = accept(listen_socket, NULL, NULL); // Accepts a potential client. Currently, this will block until a client tries to connect.
	utilities::print_line("client connected!");

	// Once we have gotten a result, check that it's valid, if so, start waiting on message (recv).
	if (client_socket > 0) {

		char buffer[1024];
		int bytes_recv = recv(client_socket, buffer, sizeof(buffer), 0);

		if (bytes_recv == -1) {
			std::cout << WSAGetLastError() << std::endl;
		}
		std::string s(buffer);
		std::cout << "Got msg: " << s << ". Consisting of " << bytes_recv << " bytes" << std::endl;

	}

}

void explainProgram() {

}

int main1() {
	
	std::cout << "Hello and Welcome to Basic Network Example (using POSIX sockets) !" << std::endl << std::endl;

	std::cout << "To run this program as a Server, type SERVER" << std::endl;
	std::cout << "To run this program as a client, type CLIENT" << std::endl;

	std::cout << "To get an explanation of what this program does, type HELP" << std::endl;

	std::cout << "To exit this program, type EXIT" << std::endl;

	int running = 1;
	std::string input;

	WSADATA wsaData;

	int res = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (res != 0) {
	    std::cout << "WSAStartup failed with error code: " << res;
	    running = 0;
	}

	while (running && getline(std::cin, input)) {

		if (input == "EXIT") { running = 0; break; }

		if (input == "HELP")   { explainProgram(); break; }
		if (input == "CLIENT") { runClient();      break; }
		if (input == "SERVER") { runServer();      break; }

	}

	WSACleanup(); // Hmm
	std::cout << "Thank you for playing Basic Network Example, please play again soon! ^-^" << std::endl;

	return 0;
}