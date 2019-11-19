#ifndef __PROXY_H
#define __PROXY_H

#include <cstdint>

class Proxy
{
	private:
		int sock;
	public:
		Proxy();
		~Proxy();
		int connect();
		int get_packet(uint8_t *buf, uint32_t *buflen);
		void disconnect();
};

#endif
