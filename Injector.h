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
} pcap_loop_args_t;

class Injector
{
private:
	MessageSink *ms { nullptr };

	void iterate_pcap(char *path);
public:
	explicit Injector(MessageSink *ms);
	~Injector() = default;

	void inject(uint32_t id);
};


#endif //V2X_INJECTOR_H
