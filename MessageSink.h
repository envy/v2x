#ifndef V2X_MESSAGESINK_H
#define V2X_MESSAGESINK_H

#include <queue>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <condition_variable>
#include <SFML/Graphics.hpp>

#include "parser.h"

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
	SPATEM_t *spatem;
	MAPEM_t *mapem;
} station_msgs_t;

class MessageSink {
private:
	std::queue<array_t> incoming;
	std::map<StationID_t, station_msgs_t *> msgs;
	std::mutex queue_lock;
	std::shared_mutex data_lock;
	std::condition_variable queue_cond;
	std::thread processor;
	bool process;

	uint32_t selected_index;
	bool _show_cams;
	bool _show_denms;
	bool _show_spatems;
	bool _show_mapems;

	void process_incoming();
	void process_msg(array_t arr);
public:
	MessageSink();
	virtual ~MessageSink();

	void add_msg(const array_t &arr);
	void inc_selected();
	void dec_selected();
	void set_show_cams(bool show);
	bool get_show_cams();
	void set_show_denms(bool show);
	bool get_show_denms();
	void set_show_spatems(bool show);
	bool get_show_spatems();
	void set_show_mapems(bool show);
	bool get_show_mapems();
	void draw_station_list();
	void draw_details();
};

#endif //V2X_MESSAGESINK_H
