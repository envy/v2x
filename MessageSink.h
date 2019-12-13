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
#include "IntersectionEntity.h"

class Array
{
	public:
	uint8_t *buf { nullptr };
	uint32_t len { 0 };
	Array(uint8_t *buf, uint32_t len) : buf(buf), len(len) {}
	~Array()
	{
		//delete[] buf;
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
	CAM_t *cam { nullptr };
	DENM_t *denm { nullptr };
	SPATEM_t *spatem { nullptr };
	MAPEM_t *mapem { nullptr };
	IntersectionEntity *ie { nullptr };
} station_msgs_t;

class MessageSink {
private:
	std::queue<Array> incoming;
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

	void process_incoming();
	void process_msg(Array &arr);

	void parse_mapem(station_msgs_t *data);
	void parse_spatem(station_msgs_t *data);
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
	void draw_details(sf::RenderTarget &target);
	void draw_map(sf::RenderTarget &background, sf::RenderTarget &foreground);
	void draw_cam(sf::RenderTarget &target, station_msgs_t *data);
	void draw_intersection(sf::RenderTarget &target, station_msgs_t *data);
};

#endif //V2X_MESSAGESINK_H
