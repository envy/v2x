#include "proxy.h"
#include "parser.h"

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

	return 0;
}

int Proxy::reliable_read(uint8_t *buf, uint32_t len)
{
	int _read = 0, ret = 0;
	while (_read < len)
	{
		ret = read(sock, buf + _read, len - _read);
		if (ret <= 0)
		{
			return ret;
		}

		_read += ret;
	}

	return _read;
}

int Proxy::get_packet(uint8_t *buf, uint32_t buflen)
{
	static const uint32_t header_size = sizeof(ethernet_t) + sizeof(geonetworking_t);

	int _read = reliable_read(buf, header_size);
	ethernet_t *e = (ethernet_t *)buf;
	geonetworking_t *g = (geonetworking_t *)e->data;
	uint32_t payload_length = ntohs(g->common_header.payload_length);
	switch (g->common_header.type.raw)
	{
		case GEONET_TYPE_SHB:
			payload_length += sizeof(geonet_shb_t);
			break;
		case GEONET_TYPE_BEACON:
			payload_length += sizeof(geonet_beacon_t);
			break;
		default:
			std::cerr << "FIXME: unknown geonet type 0x" << std::hex << (unsigned int)g->common_header.type.raw << std::dec << std::endl;
			break;
	}

	_read += reliable_read(buf + header_size, payload_length);

	return _read;
}

int Proxy::send_packet(uint8_t *buf, uint32_t len)
{
	int written = 0;
	written = write(sock, buf, len);
	//while (written < len && (written += write(sock, buf + written, len - written)) > 0)

	if (written <= 0)
	{
		std::cerr << "write error: " << strerror(errno) << std::endl;
	}

	return written;
}

void Proxy::disconnect()
{
	close(sock);
}

