#include "MessageSink.h"
#include "main.h"
#include "factory.h"
#include "Formatter.h"
#include "Utils.h"
#include "IntersectionEntity.h"

#include <iostream>
#include <sstream>
#include <asn1-src/IntersectionGeometryList.h>
#include <asn1-src/MovementEventList.h>

#include "IntersectionGeometryList.h"
#include "IntersectionGeometry.h"
#include "IntersectionState.h"
#include "GenericLane.h"
#include "LaneID.h"
#include "SignalGroupID.h"
#include "ConnectsToList.h"
#include "Connection.h"
#include "NodeListXY.h"
#include "NodeSetXY.h"
#include "NodeXY.h"
#include "NodeOffsetPointXY.h"
#include "Node-XY-20b.h"
#include "Offset-B10.h"
#include "Node-XY-22b.h"
#include "Offset-B11.h"
#include "Node-XY-24b.h"
#include "Offset-B12.h"
#include "Node-XY-26b.h"
#include "Offset-B13.h"
#include "Node-XY-28b.h"
#include "Offset-B14.h"
#include "Node-XY-32b.h"
#include "Offset-B16.h"
#include "MovementPhaseState.h"
#include "MovementState.h"
#include "MovementEventList.h"
#include "MovementEvent.h"
#include "NodeAttributeSetXY.h"
#include "NodeAttributeXYList.h"
#include "NodeAttributeXY.h"
#include "TimeChangeDetails.h"

MessageSink::MessageSink()
{
	selected_index = 0;
	_show_cams = true;
	_show_denms = true;
	_show_spatems = true;
	_show_mapems = true;
	_show_visu = true;
	_visu_only_vehicles = true;
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

void MessageSink::process_msg(Array &arr)
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
		std::cout << "packet was CAM and had len: " << arr.len << std::endl;
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
		std::cout << "packet was DENM and had len: " << arr.len << std::endl;
	}

	if (is_spatem(arr.buf, arr.len, &start))
	{
		SPATEM_t *spatem = nullptr;
		int ret = ::parse_spatem(arr.buf + start, arr.len - start, &spatem);
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
			parse_spatem(msgs[spatem->header.stationID]);
			return;
		}
		std::cout << "packet was SPATEM and had len: " << arr.len << std::endl;
	}

	if (is_mapem(arr.buf, arr.len, &start))
	{
		MAPEM_t *mapem = nullptr;
		int ret = ::parse_mapem(arr.buf + start, arr.len - start, &mapem);
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
			parse_mapem(msgs[mapem->header.stationID]);
			return;
		}
		std::cout << "packet was MAPEM and had len: " << arr.len << std::endl;
	}
}

void MessageSink::parse_mapem(station_msgs_t *data)
{
	if (data->mapem->map.intersections == nullptr || data->mapem->map.intersections->list.count == 0)
	{
		return;
	}

	if (data->mapem->map.layerID != nullptr)
	{
		std::cout << "this mapem has a layer and is id " << *data->mapem->map.layerID;
		if (data->mapem->map.layerType != nullptr)
		{
			std::cout << " and type " << *data->mapem->map.layerType;
		}
		std::cout << std::endl;
	}

	// We only draw the first intersection
	auto in = data->mapem->map.intersections->list.array[0];
	if (data->mapem->map.intersections->list.count > 1)
	{
		std::cout << "more than one intersection in mapem!" << std::endl;
	}

	if (data->ie == nullptr)
	{
		data->ie = new IntersectionEntity(in->refPoint.Long, in->refPoint.lat);
	}
	else
	{
		// don't need to parse new.
		// FIXME: check data->mapem->map.msgIssueRevision against stored and update if necessary
		return;
	}

	if (in->speedLimits != nullptr)
	{
		std::cout << "TODO: in->speedlimits" << std::endl;
	}

	for (uint32_t _l = 0; _l < in->laneSet.list.count; ++_l)
	{
		auto lane = in->laneSet.list.array[_l];

		uint64_t width = 300; // Default is 300 cm
		if (in->laneWidth != nullptr)
		{
			width = (uint64_t)*in->laneWidth;
		}

		data->ie->add_lane(lane->laneID, lane->laneAttributes);

		/*
		if (lane->ingressApproach != nullptr)
		{
			std::cout << "TODO: lane->ingressApproach " << *lane->ingressApproach << std::endl;
		}
		if (lane->egressApproach != nullptr)
		{
			std::cout << "TODO: lane->egressApproach " << *lane->egressApproach << std::endl;
		}
		//*/

		// nodes
		switch(lane->nodeList.present)
		{
			case NodeListXY_PR_NOTHING:
				break;
			case NodeListXY_PR_nodes:
			{
				for (uint32_t _n = 0; _n < lane->nodeList.choice.nodes->list.count; ++_n)
				{
					auto node = lane->nodeList.choice.nodes->list.array[_n];
					int64_t next_x = 0, next_y = 0;
					switch (node->delta.present)
					{
						case NodeOffsetPointXY_PR_NOTHING:
							continue;
						case NodeOffsetPointXY_PR_node_XY1:
						{
							auto nop = node->delta.choice.node_XY1;
							next_x = nop->x;
							next_y = nop->y;
							break;
						}
						case NodeOffsetPointXY_PR_node_XY2:
						{
							auto nop = node->delta.choice.node_XY2;
							next_x = nop->x;
							next_y = nop->y;
							break;
						}
						case NodeOffsetPointXY_PR_node_XY3:
						{
							auto nop = node->delta.choice.node_XY3;
							next_x = nop->x;
							next_y = nop->y;
							break;
						}
						case NodeOffsetPointXY_PR_node_XY4:
						{
							auto nop = node->delta.choice.node_XY4;
							next_x = nop->x;
							next_y = nop->y;
							break;
						}
						case NodeOffsetPointXY_PR_node_XY5:
						{
							auto nop = node->delta.choice.node_XY5;
							next_x = nop->x;
							next_y = nop->y;
							break;
						}
						case NodeOffsetPointXY_PR_node_XY6:
						{
							auto nop = node->delta.choice.node_XY6;
							next_x = nop->x;
							next_y = nop->y;
							break;
						}
						case NodeOffsetPointXY_PR_node_LatLon:
							continue;
						case NodeOffsetPointXY_PR_regional:
							continue;
						default:
							continue;
					}

					std::vector<NodeAttributeXY_t> attributes;
					if (node->attributes != nullptr)
					{
						if (node->attributes->localNode != nullptr)
						{
							auto &lnl = node->attributes->localNode;
							for (uint32_t lnl_i = 0; lnl_i < lnl->list.count; ++lnl_i)
							{
								attributes.emplace_back(*lnl->list.array[lnl_i]);
							}
						}

						if (node->attributes->data != nullptr)
						{
							std::cout << "TODO: node->attributes->data" << std::endl;
						}
						if (node->attributes->dWidth != nullptr)
						{
							// FIXME: Is this a delta or not?
							//width = (uint64_t)(width + *node->attributes->dWidth);
							width = (uint64_t)(*node->attributes->dWidth);
						}
					}

					data->ie->add_node(lane->laneID, next_x, next_y, width, attributes);
				}
			}
				break;
			case NodeListXY_PR_computed:
			{
				std::cout << "TODO: node list computed!" << std::endl;
			}
		}

		// connections
		if (lane->connectsTo != nullptr)
		{
			//ss << "   Connects to: ";
			for (uint32_t ct = 0; ct < lane->connectsTo->list.count; ++ct)
			{
				auto con = lane->connectsTo->list.array[ct];
				data->ie->add_connection(lane->laneID, con->connectingLane.lane, con->signalGroup);
				if (con->connectingLane.maneuver != nullptr)
				{
					std::cout << "TODO: con->connectingLane.maneuver" << std::endl;
				}
				if (con->remoteIntersection != nullptr)
				{
					std::cout << "TODO: con->remoteIntersection" << std::endl;
				}
			}
		}
	}

	data->ie->build_geometry();
}

void MessageSink::parse_spatem(station_msgs_t *data)
{
	if (data->spatem == nullptr || data->ie == nullptr)
	{
		return;
	}

	for (uint32_t it = 0; it < data->spatem->spat.intersections.list.count; ++it)
	{
		auto in = data->spatem->spat.intersections.list.array[it];
		for (uint32_t mst = 0; mst < in->states.list.count; ++mst)
		{
			auto ms = in->states.list.array[mst];
			data->ie->set_signal_group_state(ms->signalGroup, ms->state_time_speed.list.array[0]->eventState);
			if (ms->state_time_speed.list.array[0]->timing != nullptr)
			{
				auto t = ms->state_time_speed.list.array[0]->timing;
				//xer_fprint(stdout, &asn_DEF_TimeChangeDetails, t);
			}
		}
	}
}

void MessageSink::process_incoming()
{
	while(process)
	{
		{
			std::unique_lock lock(queue_lock);
			if (incoming.empty())
			{
				queue_cond.wait(lock);
				continue;
			}
			Array &msg = incoming.front();
			process_msg(msg);
			incoming.pop();
		}
	}
}

void MessageSink::add_msg(uint8_t *buf, uint32_t buflen)
{
	std::unique_lock qlock(queue_lock);
	incoming.emplace(buf, buflen);
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

void MessageSink::set_show_visu(bool show)
{
	_show_visu = show;
}
bool MessageSink::get_show_visu()
{
	return _show_visu;
}

void MessageSink::set_visu_only_vehicles(bool show)
{
	_visu_only_vehicles = show;
}
bool MessageSink::get_visu_only_vehicles()
{
	return _visu_only_vehicles;
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

void MessageSink::draw_intersection(station_msgs_t *data)
{
	if (data->ie != nullptr)
	{
		data->ie->build_geometry();
		_main->get_window()->draw(*data->ie);
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

	if (data->mapem != nullptr && _show_visu)
	{
		draw_intersection(data);
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

void MessageSink::draw_map()
{
	std::shared_lock lock(data_lock);

	sf::CircleShape snode(5);
	snode.setFillColor(sf::Color::White);
	snode.setPosition(_main->get_center_x(), _main->get_center_y());
	_main->get_window()->draw(snode);

	auto it = msgs.begin();
	while (it != msgs.end())
	{
		auto data = it->second;
		if (data->mapem != nullptr)
		{
			draw_intersection(data);
		}
		++it;
	}
}
