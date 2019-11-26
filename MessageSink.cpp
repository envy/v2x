#include "MessageSink.h"
#include "main.h"
#include "factory.h"
#include "Formatter.h"

#include <iostream>
#include <sstream>

MessageSink::MessageSink()
{
	selected_index = 0;
	_show_cams = true;
	_show_denms = true;
	_show_spatems = true;
	_show_mapems = true;
	process = true;
	processor = std::thread([this] {
		process_incoming();
	});
}

MessageSink::~MessageSink()
{
	process = false;
	{
		std::unique_lock lock(queue_lock);
		queue_cond.notify_all();
	}
	processor.join();
}

void MessageSink::process_msg(array_t arr)
{
	std::unique_lock lock(data_lock);
	uint32_t start;

	if (is_cam(arr.buf, arr.len, &start))
	{
		CAM_t *cam = nullptr;
		int ret = parse_cam(arr.buf+start, arr.len-start, &cam);
		if (ret == 0)
		{
			if (msgs.find(cam->header.stationID) == msgs.end())
			{
				msgs.insert(std::pair<StationID_t, station_msgs_t *>(cam->header.stationID, new station_msgs_t()));
			}
			if (msgs[cam->header.stationID]->cam != nullptr)
			{
				ASN_STRUCT_FREE(asn_DEF_CAM, msgs[cam->header.stationID]->cam);
			}
			msgs[cam->header.stationID]->last = timestamp_now();
			msgs[cam->header.stationID]->cam = cam;
			return;
		}
	}

	if (is_denm(arr.buf, arr.len, &start))
	{
		DENM_t *denm = nullptr;
		int ret = parse_denm(arr.buf+start, arr.len-start, &denm);
		if (ret == 0)
		{
			if (msgs.find(denm->header.stationID) == msgs.end())
			{
				msgs.insert(std::pair<StationID_t, station_msgs_t *>(denm->header.stationID, new station_msgs_t()));
			}
			if (msgs[denm->header.stationID]->denm != nullptr)
			{
				ASN_STRUCT_FREE(asn_DEF_DENM, msgs[denm->header.stationID]->denm);
			}
			msgs[denm->header.stationID]->denm = denm;
			return;
		}
	}

	if (is_spatem(arr.buf, arr.len, &start))
	{
		SPATEM_t *spatem = nullptr;
		int ret = parse_spatem(arr.buf + start, arr.len - start, &spatem);
		if (ret == 0)
		{
			if (msgs.find(spatem->header.stationID) == msgs.end())
			{
				msgs.insert(std::pair<StationID_t, station_msgs_t *>(spatem->header.stationID, new station_msgs_t()));
			}
			if (msgs[spatem->header.stationID]->spatem != nullptr)
			{
				ASN_STRUCT_FREE(asn_DEF_SPATEM, msgs[spatem->header.stationID]->spatem);
			}
			msgs[spatem->header.stationID]->spatem = spatem;
			return;
		}
	}

	if (is_mapem(arr.buf, arr.len, &start))
	{
		MAPEM_t *mapem = nullptr;
		int ret = parse_mapem(arr.buf + start, arr.len - start, &mapem);
		if (ret == 0)
		{
			if (msgs.find(mapem->header.stationID) == msgs.end())
			{
				msgs.insert(std::pair<StationID_t, station_msgs_t *>(mapem->header.stationID, new station_msgs_t()));
			}
			if (msgs[mapem->header.stationID]->mapem != nullptr)
			{
				ASN_STRUCT_FREE(asn_DEF_MAPEM, msgs[mapem->header.stationID]->mapem);
			}
			msgs[mapem->header.stationID]->mapem = mapem;
			xer_fprint(stdout, &asn_DEF_MAPEM, mapem);
			return;
		}
	}
}

void MessageSink::process_incoming()
{
	while(process)
	{
		{
			std::unique_lock lock(queue_lock);
			if (!incoming.empty())
			{
				process_msg(incoming.front());
				incoming.pop();
			}
			queue_cond.wait(lock);
		}
	}
}

void MessageSink::add_msg(const array_t &arr)
{
	std::unique_lock qlock(queue_lock);
	incoming.push(arr);
	queue_cond.notify_all();
}

void MessageSink::inc_selected()
{
	if (selected_index == UINT32_MAX || selected_index + 1 >= msgs.size())
	{
		return;
	}
	selected_index++;
}

void MessageSink::dec_selected()
{
	if (selected_index == 0)
	{
		return;
	}
	selected_index--;
}

void MessageSink::set_show_cams(bool show)
{
	_show_cams = show;
}
bool MessageSink::get_show_cams()
{
	return _show_cams;
}

void MessageSink::set_show_denms(bool show)
{
	_show_denms = show;
}
bool MessageSink::get_show_denms()
{
	return _show_denms;
}

void MessageSink::set_show_spatems(bool show)
{
	_show_spatems = show;
}
bool MessageSink::get_show_spatems()
{
	return _show_spatems;
}

void MessageSink::set_show_mapems(bool show)
{
	_show_mapems = show;
}
bool MessageSink::get_show_mapems()
{
	return _show_mapems;
}

void MessageSink::draw_station_list()
{
	std::shared_lock lock(data_lock);
	auto it = msgs.begin();
	uint32_t i = 1;
	while (it != msgs.end())
	{
		std::stringstream ss;
		if (i == selected_index + 1)
		{
			ss << "> ";
		}
		else
		{
			ss << "  ";
		}
		ss << it->first;
		_main->write_text(20, 10 + 20 * i, sf::Color::White, ss.str());
		++it;
		++i;
	}
}

void MessageSink::draw_details()
{
	std::shared_lock lock(data_lock);

	station_msgs_t *data = nullptr;

	auto it = msgs.begin();
	uint32_t i = 0;
	while (i < selected_index)
	{
		++i;
		++it;
	}
	data = it->second;

	if (data == nullptr)
	{
		return;
	}

	std::stringstream ss;
	if (data->cam != nullptr && _show_cams)
	{
		ss << Formatter::dump_cam(data->cam);
	}
	if (data->denm != nullptr && _show_denms)
	{
		ss << Formatter::dump_denm(data->denm);
	}
	if (data->spatem != nullptr && _show_spatems)
	{
		ss << Formatter::dump_spatem(data->spatem);
	}
	if (data->mapem != nullptr && _show_mapems)
	{
		ss << Formatter::dump_mapem(data->mapem);
	}

	_main->write_text(200, 30, sf::Color::White, ss.str());
}