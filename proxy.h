#ifndef __PROXY_H
#define __PROXY_H

#include <cstdint>
#include <mutex>

class Proxy
{
private:
	int sock { -1 };
	std::mutex send_lock;
	virtual int reliable_read(uint8_t *buf, uint32_t buflen);
public:
	Proxy();
	virtual ~Proxy();
	virtual bool connect(char *caddr, int port);
	virtual int get_packet(uint8_t *buf, uint32_t buflen);
	virtual int send_packet(uint8_t *buf, uint32_t len);
	virtual void disconnect();
};

class DummyProxy : public Proxy
{
private:
	int it { 0 };
	uint8_t *data { nullptr };
	size_t datalen { 0 };
	virtual int reliable_read(uint8_t *buf, uint32_t buflen) override;
public:
	DummyProxy();
	virtual ~DummyProxy();
	bool connect(char *caddr, int port) override;
	int send_packet(uint8_t *buf, uint32_t len) override;
	void disconnect() override;
};

#endif
