#include "proxy.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <iostream>

#define PORT 17565
#define SERVER "10.1.4.72"

Proxy::Proxy()
{
	sock = -1;
}

Proxy::~Proxy()
{
	if (sock < 0)
	{
		close(sock);
	}
}

int Proxy::connect()
{

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
	{
		std::cout << "error creating socket" << std::endl;
		return -1;
	}

	struct sockaddr_in server = {};
	unsigned long addr = inet_addr(SERVER);
	memcpy(&server.sin_addr, &addr, sizeof(addr));
	server.sin_family = AF_INET;
	server.sin_port = htons(PORT);

	if (::connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		std::cout << "error connecting: " << strerror(errno) << std::endl;
		return -2;
	}
}

int Proxy::get_packet(uint8_t *buf, uint32_t *buflen)
{
	if (read(sock, buf, *buflen) <= 0)
	{
		return -1;
	}
	return 0;
}

void Proxy::disconnect()
{
	close(sock);
}
