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

#include "asn1-src/Ieee1609Dot2Data.h"

Proxy::Proxy()
{
	sock = -1;
}

DummyProxy::DummyProxy()
{
	data = (uint8_t *)\
	"\xff\xff\xff\xff\xff\xff\xe2\xcb\x94\x76\x65\xe5\x89\x47\x12\x00" \
	"\x05\x01\x03\x81\x00\x40\x03\x80\x81\xae\x20\x50\x02\x80\x00\x8a" \
	"\x01\x00\x14\x00\xe2\xcb\x94\x76\x65\xe5\x79\xf6\xab\xe5\x1f\x28" \
	"\x89\x86\x06\x45\xe7\x55\x84\x6c\x0d\xca\x04\x00\xa0\x00\x07\xd1" \
	"\x00\x00\x02\x02\x94\x76\x65\xe5\xad\x4d\x40\x5a\x99\xae\x7e\xce" \
	"\x31\xf7\x21\x62\x34\x23\x00\xf6\x36\x84\x54\x58\xdc\xa0\xa2\x1f" \
	"\x83\x02\x96\x8a\x53\x33\xfa\x81\xff\xe6\x00\x40\x40\x14\x19\x81" \
	"\x42\x55\xfa\x1f\x80\x4b\xd8\xce\x00\x23\xef\xc2\x2c\x03\x70\xc6" \
	"\x70\x01\x4f\x7e\x15\x60\x1b\xd6\x40\x00\x09\xfb\xf0\x67\x00\xc1" \
	"\xb1\x9c\x00\x53\xdf\x82\xd8\x01\xf1\x8c\xe0\x02\x7e\xfc\x17\xc0" \
	"\x34\x8c\x67\x00\x16\x97\xe0\xaa\x01\x94\x63\x38\x01\x86\xbf\x06" \
	"\x50\x0b\x93\x19\xc0\x0b\x1d\xf8\x40\x80\x5e\xd8\x9c\x00\x3b\x6f" \
	"\xc2\x54\x03\xbe\xc6\x70\x01\xd2\x40\x01\x24\x00\x01\xda\x84\x6b" \
	"\x95\x5a\x79\x81\x01\x01\x80\x03\x00\x80\xe3\xcf\xa8\x5c\xab\x0a" \
	"\xd1\xfe\x10\x83\x00\x00\x00\x00\x00\x1f\x10\x82\x35\x84\x00\xa8" \
	"\x01\x02\x80\x01\x24\x81\x04\x03\x01\x00\x00\x80\x01\x25\x81\x05" \
	"\x04\x01\x90\x1a\x25\x80\x80\x82\xcf\x78\xaa\x5e\x33\xe7\xbf\x6e" \
	"\x9a\xd5\x17\xfd\xc4\xb9\x9a\x7f\x62\x98\x3e\x86\xca\x30\x23\x61" \
	"\xba\xaf\xad\x98\x4e\xed\x1c\xcf\x80\x80\xe5\x81\xc7\x55\x86\xec" \
	"\x91\x83\x5c\xfb\x8a\x19\x90\x34\x5d\xdf\xc0\x6a\x29\x25\xdc\x0d" \
	"\x5b\x1b\x23\xd9\xaf\x5c\x27\x5b\x96\x4a\xdc\xab\x0e\x16\xb9\x32" \
	"\xb5\x65\xea\x3b\x40\x2e\x2a\x1e\x26\xd1\x66\x07\xa6\x33\x35\x6f" \
	"\x67\x0e\x7c\x71\x7e\xd0\x99\x0e\xd0\xf8\x80\x82\x5f\x83\xb7\x82" \
	"\x9b\xf1\x73\x68\x58\x9a\x64\xf5\x65\xc9\xfb\x3c\x48\x44\x71\xbd" \
	"\xbf\xce\x9c\xad\xc5\x93\xde\xa0\xe9\xa2\x5b\x3f\xc7\x57\x47\xab" \
	"\x66\x02\x85\xaf\xaa\x6a\xd6\xa3\xc8\xdd\x33\x91\x2f\xdc\xce\x53" \
	"\x40\x72\x68\xe3\xf5\x19\x02\x00\xba\x43\xf3\xeb" \
	"\xff\xff\xff\xff\xff\xff\xe2\xcb\x94\x76\x65\xe5\x89\x47\x12\x00" \
	"\x05\x01\x03\x81\x00\x40\x03\x80\x56\x20\x50\x02\x80\x00\x32\x01" \
	"\x00\x14\x00\xe2\xcb\x94\x76\x65\xe5\x79\xf6\xab\xe5\x1f\x28\x89" \
	"\x86\x06\x45\xe7\x55\x84\x6c\x0d\xca\x02\x00\xa0\x00\x07\xd1\x00" \
	"\x00\x02\x02\x94\x76\x65\xe5\xae\xdc\x00\x5a\x99\xae\xae\x0e\x31" \
	"\xf7\x17\x82\x34\x23\x01\x00\x36\x84\x54\x58\xdc\x80\xa2\x09\x03" \
	"\x02\x96\x8a\x57\x33\xff\x82\x00\xae\x00\x40\x80\x14\x39\x80\x40" \
	"\x01\x24\x00\x01\xda\x84\x6b\x9b\x73\x45\x80\x25\x6a\xe0\xcb\x94" \
	"\x76\x65\xe5\x80\x82\xd4\xb8\x19\x80\xfb\x90\xf1\x12\x86\xd5\x61" \
	"\x6b\xf2\x59\x2e\xc8\xb8\x8c\x56\x1c\xc7\x11\x40\x6e\x12\x52\x68" \
	"\xda\xba\x90\xf5\xd4\xa8\xd8\xcc\x6c\x92\xcc\x8f\xc7\x08\x47\x39" \
	"\x13\x71\x3f\x22\x96\xb7\x1b\xfd\x93\x42\xa2\x30\x6d\xbe\x85\xb9" \
	"\x27\xb8\x4f\x0a\xa6";

	datalen = 428 + 197;
}

Proxy::~Proxy()
{
	disconnect();
}

DummyProxy::~DummyProxy()
{
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

bool DummyProxy::connect(char *caddr, int port)
{
	(void)caddr;
	(void)port;
	return true;
}

int Proxy::reliable_read(uint8_t *buf, uint32_t len)
{
	int _read = 0;
	ssize_t ret = 0;
	while (_read < (int)len)
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

int DummyProxy::reliable_read(uint8_t *buf, uint32_t buflen)
{
	if (it + buflen > datalen)
	{
		sleep(100000);
	}
	memcpy(buf, data + it, buflen);
	it += buflen;
	return buflen;
}

int Proxy::get_packet(uint8_t *buf, uint32_t buflen)
{
	uint32_t header_size = sizeof(ethernet_t);

	int _read = reliable_read(buf, header_size);
	auto *e = (ethernet_t *)buf;

	if (e->type != ETHERTYPE_GEONET)
	{
		// This is not a geonetworking packet.
		// FIXME: how do we proceed? we don't know how long this packet is...
		// We could try reading byte by byte until we see the magic geonet type number?
		// Also, the dest mac should always be ff:ff:ff:ff:ff:ff, so we have 8 bytes to look for.
		return _read;
	}

	header_size += sizeof(geonetworking_t);
	_read += reliable_read(buf + header_size, sizeof(geonetworking_t));

	auto *g = (geonetworking_t *)e->data;
	if (g->basic_header.next_header == GEONET_BASIC_HEADER_NEXT_ANY)
	{
		return _read;
	}
	if (g->basic_header.next_header == GEONET_BASIC_HEADER_NEXT_COMMON)
	{
		auto *c = (geonetworking_common_header_t *)g->data;
		_read += reliable_read(buf + header_size, sizeof(geonetworking_common_header_t));
		header_size += sizeof(geonetworking_common_header_t);

		uint32_t payload_length = ntohs(c->payload_length);
		switch (c->type.raw)
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
				std::cerr << "PROXY FIXME: unknown geonet type 0x" << std::hex << (unsigned int)c->type.raw << std::dec << std::endl;
				payload_length += 0;
				break;
		}

		_read += reliable_read(buf + header_size, payload_length);
		return _read;
	}
	else if (g->basic_header.next_header == GEONET_BASIC_HEADER_NEXT_SECURED)
	{
		asn_dec_rval_t ret;
		size_t len = 7;
		_read += reliable_read(buf + header_size, 7); // first read 7 bytes to start

		Ieee1609Dot2Data_t *data = nullptr;

		while (true)
		{
			ASN_STRUCT_RESET(asn_DEF_Ieee1609Dot2Data, data);
			ret = oer_decode(nullptr, &asn_DEF_Ieee1609Dot2Data, (void **)&data, g->data, len);

			if (ret.code == RC_WMORE)
			{
				int __read = reliable_read(buf + header_size + len, 1);
				_read += __read;
				len += __read;
				continue;
			}
			if (ret.code == RC_FAIL)
			{
				ASN_STRUCT_FREE(asn_DEF_Ieee1609Dot2Data, data);
				std::cerr << "asn decode failure" << std::endl;
				break;
			}
			if (ret.code == RC_OK)
			{
				ASN_STRUCT_FREE(asn_DEF_Ieee1609Dot2Data, data);
				break;
			}
		}

		return _read;
	}
	else
	{
		std::cerr << "unknown next header " << (int)g->basic_header.next_header << std::endl;
	}
	return 0;
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

int DummyProxy::send_packet(uint8_t *buf, uint32_t len)
{
	(void)buf;
	return len;
}

void Proxy::disconnect()
{
	if (sock >= 0)
	{
		close(sock);
		sock = -1;
	}
}

void DummyProxy::disconnect()
{
}
