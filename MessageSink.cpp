#include "MessageSink.h"
#include "main.h"
#include "factory.h"
#include "Formatter.h"
#include "Utils.h"
#include "IntersectionEntity.h"

#include <iostream>
#include <sstream>
#include <asn1-src/NodeAttributeXYList.h>

#include "IntersectionGeometryList.h"
#include "IntersectionGeometry.h"
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
#include "NodeAttributeSetXY.h"
#include "NodeAttributeXYList.h"
#include "NodeAttributeXY.h"

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
		std::cout << "packet was SPATEM and had len: " << arr.len << std::endl;
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
			parse_mapm(msgs[mapem->header.stationID]);
			return;
		}
		std::cout << "packet was MAPEM and had len: " << arr.len << std::endl;
	}
}

void MessageSink::parse_mapm(station_msgs_t *data)
{
	std::map<LaneID_t, intersection_data_t> intersection;

	if (data->mapem->map.intersections == nullptr || data->mapem->map.intersections->list.count == 0)
	{
		return;
	}

	// We only draw the first intersection
	auto in = data->mapem->map.intersections->list.array[0];

	if (data->ie == nullptr)
	{
		data->ie = new IntersectionEntity(in->refPoint.Long, in->refPoint.lat);
	}
	else
	{
		// don't need to parse new.
		return;
	}

	for (uint32_t _l = 0; _l < in->laneSet.list.count; ++_l)
	{
		auto lane = in->laneSet.list.array[_l];
		if (_visu_only_vehicles && lane->laneAttributes.laneType.present != LaneTypeAttributes_PR_vehicle)
		{
			continue;
		}

		uint64_t width = 300;
		if (in->laneWidth != nullptr)
		{
			width = (uint64_t)*in->laneWidth;
		}

		/*
		if (Utils::is_ingress_lane(lane->laneAttributes.directionalUse))
		{
			snode.setFillColor(sf::Color::Blue);
		}
		if (Utils::is_egress_lane(lane->laneAttributes.directionalUse))
		{
			snode.setFillColor(sf::Color::Red);
		}
		//*/

		uint64_t l = data->ie->add_lane(width);

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

					bool is_stopline = false;
					if (node->attributes != nullptr)
					{
						if (node->attributes->localNode != nullptr)
						{
							auto &lnl = node->attributes->localNode;
							for (uint32_t lnl_i = 0; lnl_i < lnl->list.count; ++lnl_i)
							{
								auto ln = lnl->list.array[lnl_i];
								switch (*ln)
								{
									case NodeAttributeXY_stopLine:
									{
										is_stopline = true;
										break;
									}
									default:
										std::cout << "unhandled node attribute: " << *ln << std::endl;
										break;
								}
							}
						}
					}

					data->ie->add_node(l, next_x, next_y, is_stopline);

				}
			}
				break;
			case NodeListXY_PR_computed:
			{
				std::cout << "node list computed!" << std::endl;
			}
		}
	}

	//_main->get_window()->draw(*data->ie);

	// Iterate again for connections
	for (uint32_t _l = 0; _l < in->laneSet.list.count; ++_l)
	{
		auto lane = in->laneSet.list.array[_l];
		if (_visu_only_vehicles && lane->laneAttributes.laneType.present != LaneTypeAttributes_PR_vehicle)
		{
			continue;
		}

		if (lane->connectsTo != nullptr)
		{
			//ss << "   Connects to: ";
			for (uint32_t ct = 0; ct < lane->connectsTo->list.count; ++ct)
			{
				auto con = lane->connectsTo->list.array[ct];
				//ss << con->connectingLane.lane;
				auto &s = intersection[lane->laneID];
				auto &e = intersection[con->connectingLane.lane];

				sf::Vertex line[] = {sf::Vertex(sf::Vector2f(s.first_node.x, s.first_node.y)), sf::Vertex(sf::Vector2f(e.first_node.x, e.first_node.y))};

				if (con->signalGroup != nullptr)
				{
					auto phase = Utils::get_movement_phase_for_signal_group(data, *con->signalGroup);
					switch (phase)
					{
						default:
							break;
						case MovementPhaseState_stop_And_Remain:
						{
							line[0].color = sf::Color::Red;
							line[1].color = sf::Color::Red;
							break;
						}
						case MovementPhaseState_pre_Movement:
						{
							line[0].color = sf::Color::Yellow;
							line[1].color = sf::Color::Red;
							break;
						}
						case MovementPhaseState_protected_Movement_Allowed:
						case MovementPhaseState_permissive_Movement_Allowed:
						{
							line[0].color = sf::Color::Green;
							line[1].color = sf::Color::Green;
							break;
						}
						case MovementPhaseState_protected_clearance:
						case MovementPhaseState_permissive_clearance:
						{
							line[0].color = sf::Color::Yellow;
							line[1].color = sf::Color::Yellow;
							break;
						}
					}
				}

				_main->get_window()->draw(line, 2, sf::Lines);
			}
		}
	}

	data->ie->build_geometry();
}

void MessageSink::process_incoming()
{
	while(process)
	{
		array_t msg;
		{
			std::unique_lock lock(queue_lock);
			if (incoming.empty())
			{
				queue_cond.wait(lock);
				continue;
			}
			msg = incoming.front();
			incoming.pop();
		}
		process_msg(msg);
		//free(msg.buf);
	}
}

void MessageSink::add_msg(uint8_t *buf, uint32_t buflen)
{
	std::unique_lock qlock(queue_lock);
	incoming.push({ buf, buflen});
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
	sf::CircleShape snode(5);
	snode.setFillColor(sf::Color::White);
	snode.setPosition(_main->get_center_x(), _main->get_center_y());
	_main->get_window()->draw(snode);

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
