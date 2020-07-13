#ifndef V2X_INJECTOR_H
#define V2X_INJECTOR_H

#include <pcap.h>
#include "MessageSink.h"

typedef struct
{
	char *name;
	char *path;
} injectable_msg_t;

extern injectable_msg_t injectable_msgs[];
extern const uint32_t max_id;

class Injector
{
private:
	MessageSink *ms { nullptr };
	bool injecting { false };
	bool thread_injecting { false };
	uint32_t inject_msg_counter { 0 };
	float time_factor { 1.0 };
	uint32_t max_usleep_time { 2 * 1'000'000 };
	std::thread *injector_thread { nullptr };

	void iterate_pcap(char *path);
	static void pcap_loop_callback(u_char *userData, const struct pcap_pkthdr *pkthdr, const u_char *packet);
public:
	explicit Injector(MessageSink *ms);
	~Injector() = default;

	void inject(uint32_t id);
	void stop_injecting();
	bool is_injecting() { return injecting; }
	uint32_t get_counter() { return inject_msg_counter; }
	void inc_counter() { inject_msg_counter++; }
	float get_time_factor() { return time_factor; }
	void set_time_factor(float tf) { time_factor = tf; }
	uint32_t get_max_usleep_time() { return max_usleep_time; }
	void set_max_usleep_time(uint32_t t) { max_usleep_time = t; }
};


typedef struct
{
	MessageSink *ms;
	struct timeval last;
	Injector *i;
	pcap_t *fp;
} pcap_loop_args_t;

#endif //V2X_INJECTOR_H
