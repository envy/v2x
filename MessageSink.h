#ifndef V2X_MESSAGESINK_H
#define V2X_MESSAGESINK_H

#include <queue>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <condition_variable>

#include "parser.h"
#include "IntersectionEntity.h"
#include "VehicleEntity.h"

class Array
{
	public:
	uint8_t *buf { nullptr };
	uint32_t len { 0 };
	Array(uint8_t *buf, uint32_t len) : buf(buf), len(len) {}
	~Array()
	{
		delete[] buf;
	}
	Array(Array &&a) noexcept
	{
		buf = a.buf;
		a.buf = nullptr;
		len = a.len;
		a.len = 0;
	}
	Array &operator=(Array &&a) noexcept
	{
		delete[] buf;

		buf = a.buf;
		a.buf = nullptr;
		len = a.len;
		a.len = 0;

		return *this;
	}
};

typedef struct
{
	uint64_t last { 0 };
	uint8_t cam_version { 0 };
	union {
		CAMv1_t *v1;
		CAM_t *v2;
	} cam { nullptr };
	uint8_t denm_version { 0 };
	union {
		DENMv1_t *v1;
		DENM_t *v2;
	} denm { nullptr };
	SPATEM_t *spatem { nullptr };
	MAPEM_t *mapem { nullptr };
	IntersectionEntity *ie { nullptr };
	VehicleEntity *ve { nullptr };
} station_msgs_t;

class MessageSink {
protected:
	std::queue<Array> incoming;
	std::map<StationID_t, station_msgs_t *> msgs;
	std::mutex queue_lock;
	std::shared_mutex data_lock;
	std::condition_variable queue_cond;
	std::thread processor;
	bool process { true };

	void process_incoming();
	void process_msg(Array &arr);

	void parse_cam(station_msgs_t *data);
	void parse_mapem(station_msgs_t *data);
	void parse_spatem(station_msgs_t *data);
public:
	MessageSink();
	virtual ~MessageSink();

	void add_msg(uint8_t *buf, uint32_t buflen);
	void add_msg_static(uint8_t *buf, uint32_t buflen);
};

#endif //V2X_MESSAGESINK_H
