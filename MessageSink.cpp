#include "MessageSink.h"
#include "main.h"
#include "factory.h"
#include "Formatter.h"
#include "Utils.h"

#include <iostream>
#include <sstream>

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

MessageSink::MessageSink()
{
	selected_index = 0;
	scale = 25.0;
	origin_x = 0;
	origin_y = 0;
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
			return;
		}
		std::cout << "packet was MAPEM and had len: " << arr.len << std::endl;
	}
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
	float x, y, prev_x, prev_y;
	bool is_first;
	auto lane_color = sf::Color(100, 100, 100);
	auto lane_outer_color = sf::Color(200, 200, 200);
	std::map<LaneID_t, intersection_data_t> intersection;

	sf::CircleShape snode(5);
	snode.setFillColor(sf::Color::White);
	snode.setPosition(CENTER_X, CENTER_Y);
	_main->get_window()->draw(snode);

	if (data->mapem->map.intersections == nullptr || data->mapem->map.intersections->list.count == 0)
	{
		return;
	}

	// We only draw the first intersection
	auto in = data->mapem->map.intersections->list.array[0];

	for (uint32_t _l = 0; _l < in->laneSet.list.count; ++_l)
	{
		auto lane = in->laneSet.list.array[_l];
		if (_visu_only_vehicles && lane->laneAttributes.laneType.present != LaneTypeAttributes_PR_vehicle)
		{
			continue;
		}
		//ss << "  Lane: " << lane->laneID << std::endl;
		//ss << "   Direction: " << format_lane_direction(lane->laneAttributes.directionalUse);
		//ss << "   Type: " << format_lane_type(lane->laneAttributes.laneType);
		float width = 330.0f / scale;
		if (in->laneWidth != nullptr)
		{
			// laneWidth in cm, so 330 = 3.3m
			width = *in->laneWidth / scale;
			//std::cout << "width: " << *in->laneWidth << std::endl;
		}
		float half_width = width / 2.0f;

		snode.setFillColor(sf::Color::Yellow);
		snode.setPosition(CENTER_X + (in->refPoint.Long - origin_x) / scale, CENTER_Y - (in->refPoint.lat - origin_y) / scale);
		_main->get_window()->draw(snode);

		if (Utils::is_ingress_lane(lane->laneAttributes.directionalUse))
		{
			snode.setFillColor(sf::Color::Blue);
		}
		if (Utils::is_egress_lane(lane->laneAttributes.directionalUse))
		{
			snode.setFillColor(sf::Color::Red);
		}

		x = CENTER_X + (in->refPoint.Long - origin_x) / scale;
		y = CENTER_Y - (in->refPoint.lat - origin_y) / scale;
		is_first = true;
		switch(lane->nodeList.present)
		{
			case NodeListXY_PR_NOTHING:
				break;
			case NodeListXY_PR_nodes:
			{
				for (uint32_t _n = 0; _n < lane->nodeList.choice.nodes->list.count; ++_n)
				{
					auto node = lane->nodeList.choice.nodes->list.array[_n];
					float next_x, next_y;
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
					prev_x = x;
					prev_y = y;
					x = (float)(x + (next_x / scale));
					y = (float)(y - (next_y / scale));
					snode.setPosition(x, y);
					if (is_first)
					{
						auto &d = intersection[lane->laneID];
						d.first_node.x = x;
						d.first_node.y = y;
						is_first = false;
					}
					else
					{
						// connect both nodes
						auto start = sf::Vector2f(prev_x, prev_y);
						auto end = sf::Vector2f(x, y);
						auto dir = end-start;
						sf::ConvexShape road;
						road.setPointCount(4);
						road.setFillColor(lane_color);
						road.setOutlineColor(lane_outer_color);
						road.setOutlineThickness(1);
						road.setPoint(0, start - Utils::ortho(dir) * half_width);
						road.setPoint(1, start + Utils::ortho(dir) * half_width);
						road.setPoint(2, end + Utils::ortho(dir) * half_width);
						road.setPoint(3, end - Utils::ortho(dir) * half_width);
						_main->get_window()->draw(road);

						//sf::Vertex line[] = {sf::Vertex(start), sf::Vertex(end)};
						//_main->get_window()->draw(line, 2, sf::Lines);
					}
					//_main->get_window()->draw(snode);
				}
			}
			break;
			case NodeListXY_PR_computed:
			{
				std::cout << "node list computed!" << std::endl;
			}
		}
	}

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

float MessageSink::get_zoom()
{
	return scale;
}

void MessageSink::set_origin(uint32_t lat, uint32_t lon)
{
	// lon is x, lat is y
	origin_x = lon;
	origin_y = lat;
}

void MessageSink::get_origin(uint32_t *lat, uint32_t *lon)
{
	*lon = origin_x;
	*lat = origin_y;
}

void MessageSink::move(uint32_t lat_delta, uint32_t lon_delta)
{
	origin_x += lon_delta;
	origin_y += lat_delta;
}

void MessageSink::zoom(float step)
{
	scale += step;
}
