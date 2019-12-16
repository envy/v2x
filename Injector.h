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

typedef struct
{
	MessageSink *ms;
	struct timeval last;
	uint32_t *counter;
} pcap_loop_args_t;

class Injector
{
private:
	MessageSink *ms { nullptr };
	bool injecting { false };
	uint32_t inject_msg_counter { 0 };

	void iterate_pcap(char *path);
public:
	explicit Injector(MessageSink *ms);
	~Injector() = default;

	void inject(uint32_t id);
	bool is_injecting() { return injecting; };
	uint32_t get_counter() { return inject_msg_counter; };
};


#endif //V2X_INJECTOR_H
