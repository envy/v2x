#ifndef __PROXY_H
#define __PROXY_H

#include <cstdint>

class Proxy
{
	private:
		int sock;
		int reliable_read(uint8_t *buf, uint32_t buflen);
	public:
		Proxy();
		~Proxy();
		int connect(char *caddr, int port);
		int get_packet(uint8_t *buf, uint32_t buflen);
		int send_packet(uint8_t *buf, uint32_t len);
		void disconnect();
};

#endif
