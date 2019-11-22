#include "MessageSink.h"
#include "main.h"
#include "factory.h"

#include <iostream>
#include <sstream>

MessageSink::MessageSink()
{
	process = true;
	processor = std::thread([this] {
		process_incoming();
	});
}

MessageSink::~MessageSink()
{
	process = false;
	{
		std::unique_lock<std::mutex> lock(queue_lock);
		queue_cond.notify_all();
	}
	processor.join();
}

void MessageSink::add_msg(const array_t &arr)
{
	std::unique_lock<std::mutex> lock(queue_lock);
	incoming.push(arr);
	queue_cond.notify_all();
}

void MessageSink::process_msg(array_t arr)
{
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
			dump_cam(cam);
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

	if (is_spat(arr.buf, arr.len, &start))
	{
		SPATEM_t *spat = nullptr;
		int ret = parse_spat(arr.buf+start, arr.len-start, &spat);
		if (ret == 0)
		{
			if (msgs.find(spat->header.stationID) == msgs.end())
			{
				msgs.insert(std::pair<StationID_t, station_msgs_t *>(spat->header.stationID, new station_msgs_t()));
			}
			if (msgs[spat->header.stationID]->spat != nullptr)
			{
				ASN_STRUCT_FREE(asn_DEF_SPATEM, msgs[spat->header.stationID]->spat);
			}
			msgs[spat->header.stationID]->spat = spat;
			return;
		}
	}
}

void MessageSink::process_incoming()
{
	while(process)
	{
		{
			std::unique_lock<std::mutex> lock(queue_lock);
			if (!incoming.empty())
			{
				process_msg(incoming.front());
				incoming.pop();
			}
			queue_cond.wait(lock);
		}
	}
}

void MessageSink::draw_data()
{
	auto it = msgs.begin();
	uint32_t i = 1;
	while (it != msgs.end())
	{
		std::stringstream ss;
		ss << it->first;
		write(20, 20*i, sf::Color::White, ss.str());
		std::cout << "." << std::flush;
		++it;
		++i;
	}
}
