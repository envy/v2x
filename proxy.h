#ifndef __PROXY_H
#define __PROXY_H

#include <cstdint>
#include <mutex>

class Proxy
{
	private:
		int sock;
		int reliable_read(uint8_t *buf, uint32_t buflen);
		std::mutex send_lock;
	public:
		Proxy();
		~Proxy();
		int connect(char *caddr, int port);
		int get_packet(uint8_t *buf, uint32_t buflen);
		int send_packet(uint8_t *buf, uint32_t len);
		void disconnect();
};

#endif
