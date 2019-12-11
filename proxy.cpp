#include "proxy.h"
#include "parser.h"

#include <cstdlib>
#include <unistd.h>
#include <cerrno>
#include <string>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <iostream>

Proxy::Proxy()
{
	sock = -1;
}

Proxy::~Proxy()
{
	disconnect();
}

bool Proxy::connect(char *caddr, int port)
{

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
	{
		std::cerr << "error creating socket" << std::endl;
		return false;
	}

	//int flags = 1;
	//setsockopt(sock, SOL_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags));

	struct sockaddr_in server = {};
	unsigned long addr = inet_addr(caddr);
	memcpy(&server.sin_addr, &addr, sizeof(addr));
	server.sin_family = PF_INET;
	server.sin_port = htons(port);

	if (::connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		std::cerr << "error connecting: " << strerror(errno) << std::endl;
		return false;
	}

	return true;
}

int Proxy::reliable_read(uint8_t *buf, uint32_t len)
{
	int _read = 0;
	ssize_t ret = 0;
	while (_read < len)
	{
		ret = read(sock, buf + _read, len - _read);
		if (ret <= 0)
		{
			return (int)ret;
		}

		_read += ret;
	}

	return _read;
}

int Proxy::get_packet(uint8_t *buf, uint32_t buflen)
{
	static const uint32_t header_size = sizeof(ethernet_t) + sizeof(geonetworking_t);

	int _read = reliable_read(buf, header_size);
	auto *e = (ethernet_t *)buf;
	auto *g = (geonetworking_t *)e->data;
	uint32_t payload_length = ntohs(g->common_header.payload_length);
	switch (g->common_header.type.raw)
	{
		case GEONET_TYPE_BEACON:
			payload_length += sizeof(geonet_beacon_t);
			break;
		case GEONET_TYPE_GUC:
			payload_length += sizeof(geonet_guc_t);
			break;
		case GEONET_TYPE_GAC_CIRCLE:
		case GEONET_TYPE_GAC_ELLIPSE:
		case GEONET_TYPE_GAC_RECT:
		case GEONET_TYPE_GBC_CIRCLE:
		case GEONET_TYPE_GBC_ELLIPSE:
		case GEONET_TYPE_GBC_RECT:
			payload_length += sizeof(geonet_gac_t);
			break;
		case GEONET_TYPE_TSB_MHB:
			payload_length += sizeof(geonet_tsb_mhb_t);
			break;
		case GEONET_TYPE_TSB_SHB:
			payload_length += sizeof(geonet_tsb_shb_t);
			break;
		default:
			std::cerr << "FIXME: unknown geonet type 0x" << std::hex << (unsigned int)g->common_header.type.raw << std::dec << std::endl;
			payload_length += 0;
			break;
	}

	_read += reliable_read(buf + header_size, payload_length);
	std::cout << "read packet with len " << _read << std::endl;

	return _read;
}

int Proxy::send_packet(uint8_t *buf, uint32_t len)
{
    std::lock_guard<std::mutex> lock(send_lock);
	int written = 0;
	//written = write_text(sock, buf, len);
	while (written < len && (written += write(sock, buf + written, len - written)) > 0) {}
	if (written <= 0)
	{
		std::cerr << "write_text error: " << strerror(errno) << std::endl;
	}

	usleep(100*1000);

	return written;
}

void Proxy::disconnect()
{
	if (sock >= 0)
	{
		close(sock);
		sock = -1;
	}
}

