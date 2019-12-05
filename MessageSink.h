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

typedef struct
{
	struct {
		float x;
		float y;
	} first_node;
} intersection_data_t;

class MessageSink {
private:
	static constexpr float center_x = 1000;
	static constexpr float center_y = 500;
	float scale;
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
	bool _show_visu;
	bool _visu_only_vehicles;
	uint32_t origin_x, origin_y;

	void process_incoming();
	void process_msg(array_t arr);
public:
	MessageSink();
	virtual ~MessageSink();

	void add_msg(uint8_t *buf, uint32_t buflen);
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
	void set_show_visu(bool show);
	bool get_show_visu();
	void set_visu_only_vehicles(bool show);
	bool get_visu_only_vehicles();
	void draw_station_list();
	void draw_details();
	void draw_intersection(station_msgs_t *data);
	void set_origin(uint32_t lat, uint32_t lon);
	void zoom(float step);
};

#endif //V2X_MESSAGESINK_H
