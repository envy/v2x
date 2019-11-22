#ifndef V2X_MESSAGESINK_H
#define V2X_MESSAGESINK_H

#include <queue>
#include <map>
#include <mutex>
#include <thread>
#include <condition_variable>
#include "parser.h"
#include "CAM.h"
#include "DENM.h"
#include "SPATEM.h"

typedef struct
{
	uint8_t *buf;
	uint32_t len;
} array_t;

typedef struct
{
	uint64_t last;
	CAM_t *cam;
	DENM_t *denm;
	SPATEM_t *spat;
} station_msgs_t;

class MessageSink {
private:
	std::queue<array_t> incoming;
	std::map<StationID_t, station_msgs_t *> msgs;
	std::mutex queue_lock;
	std::condition_variable queue_cond;
	std::thread processor;
	bool process;

	void process_incoming();
	void process_msg(array_t arr);
public:
	MessageSink();
	virtual ~MessageSink();

	void add_msg(const array_t &arr);
};


#endif //V2X_MESSAGESINK_H
