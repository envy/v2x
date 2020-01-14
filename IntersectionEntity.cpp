#include <SFML/Graphics/Vertex.hpp>
#include "IntersectionEntity.h"
#include "main.h"
#include "Utils.h"

#include <iostream>
#include <cmath>

Lane::~Lane()
{
	// FIXME: refer to fixme above
	delete[] attr.directionalUse.buf;
	delete ingress_arrow;
	delete egress_arrow;
}

Lane::Lane(Lane &&l) noexcept
{
	id = l.id;
	l.id = -1;
	attr = l.attr;
	l.attr.directionalUse.buf = nullptr;
	ingress_arrow = l.ingress_arrow;
	l.ingress_arrow = nullptr;
	egress_arrow = l.egress_arrow;
	l.egress_arrow = nullptr;
	intersection = l.intersection;
	l.intersection = nullptr;
	connections = std::move(l.connections);
	nodes = std::move(l.nodes);
	is_ingress = l.is_ingress;
	is_egress = l.is_egress;
	can_turn_left = l.can_turn_left;
	can_turn_straight = l.can_turn_straight;
	can_turn_right = l.can_turn_right;
}

Lane &Lane::operator=(Lane &&l) noexcept
{
	if (this == &l)
	{
		return *this;
	}

	delete[] attr.directionalUse.buf;
	delete ingress_arrow;
	delete egress_arrow;

	id = l.id;
	l.id = -1;
	attr = l.attr;
	l.attr.directionalUse.buf = nullptr;
	ingress_arrow = l.ingress_arrow;
	l.ingress_arrow = nullptr;
	egress_arrow = l.egress_arrow;
	l.egress_arrow = nullptr;
	intersection = l.intersection;
	l.intersection = nullptr;
	connections = std::move(l.connections);
	nodes = std::move(l.nodes);
	is_ingress = l.is_ingress;
	is_egress = l.is_egress;
	can_turn_left = l.can_turn_left;
	can_turn_straight = l.can_turn_straight;
	can_turn_right = l.can_turn_right;

	return *this;
}

void Lane::draw(sf::RenderTarget &target, sf::RenderStates states) const
{
	if (ingress_arrow != nullptr)
		target.draw(*ingress_arrow, states);
	if (egress_arrow != nullptr)
		target.draw(*egress_arrow, states);
	if (stopline != nullptr)
		target.draw(*stopline, states);

	std::for_each(connections.begin(), connections.end(), [&target, &states](const LaneConnection &lc) {
		target.draw(lc, states);
	});
}

sf::Vector2<int64_t> IntersectionEntity::get_location()
{
	return sf::Vector2<int64_t>(ref_x, ref_y);
}

void IntersectionEntity::add_lane(LaneID_t id, LaneAttributes_t &attr)
{
	Lane l;
	// FIXME: LaneAttributes contain a BITSTRING in a substruct which holds a pointer to a buffer
	// FIXME: This buffer gets deleted, when a new MAPEM is received.
	// FIXME: find a nicer way than this:
	delete[] l.attr.directionalUse.buf; // nullptr check unnecessary // FIXME: see fixme in dtor
	l.attr = attr;
	l.attr.directionalUse.buf = new uint8_t[attr.directionalUse.size];
	l.intersection = this;
	l.id = id;
	memcpy(l.attr.directionalUse.buf, attr.directionalUse.buf, l.attr.directionalUse.size);
	// FIXME: copy missing for LaneSharing and LaneTypeAttributes
	lanes[id] = std::move(l);
}

void IntersectionEntity::add_node(LaneID_t lane_id, int64_t x, int64_t y, uint64_t width, std::vector<NodeAttributeXY_t> &attributes)
{
	auto &lane = lanes[lane_id];
	if (lane.nodes.empty())
	{
		//first node is referenced from ref-point
		LaneNode n;
		//n.x = ref_x + x;
		//n.y = ref_y + y;
		Utils::lat_lon_move(ref_y, ref_x, x, y, n.y, n.x);
		n.width = width;
		n.attributes = std::move(attributes);
		lane.nodes.emplace_back(std::move(n));
	}
	else
	{
		//every other node is referenced from the previous node
		auto &prev_node = lane.nodes[lane.nodes.size() - 1];
		LaneNode n;
		//n.x = prev_node.x + x;
		//n.y = prev_node.y + y;
		Utils::lat_lon_move(prev_node.y, prev_node.x, x, y, n.y, n.x);
		n.width = width;
		n.attributes = std::move(attributes);
		lane.nodes.emplace_back(std::move(n));
	}
}

void IntersectionEntity::build_geometry(bool standalone)
{
	if (_main == nullptr)
	{
		return;
	}
	lane_outline_geometries.clear();
	lane_geometries.clear();
	lane_nodes.clear();

	sf::Vector2f coords;
	sf::Vector2<int64_t> refp;
	if (standalone)
	{
		refp = sf::Vector2<int64_t>(ref_x, ref_y);
	}
	else
	{
		refp = _main->get_origin();
	}

	// RSU
	auto r = 100/_main->get_scale();
	if (ref == nullptr)
	{
		ref = new sf::CircleShape(r);
		dynamic_cast<sf::CircleShape *>(ref)->setFillColor(sf::Color::Cyan);
	}

	dynamic_cast<sf::CircleShape *>(ref)->setRadius(r);
	dynamic_cast<sf::CircleShape *>(ref)->setOrigin(r, r);
	dynamic_cast<sf::CircleShape *>(ref)->setPosition(Utils::to_screen(ref_x, ref_y, refp.x, refp.y));

	auto lit = lanes.begin();
	while (lit != lanes.end())
	{
		auto &laneobj = lit->second;

		sf::VertexArray lane(sf::TriangleStrip);
		sf::VertexArray lane_outline(sf::TriangleStrip);
		sf::VertexArray lane_node_strip(sf::LineStrip);

		int64_t lx, ly, plx, ply;
		lx = ref_x;
		ly = ref_y;

		auto this_lane_color = sf::Color::White;
		switch (laneobj.attr.laneType.present)
		{
			case LaneTypeAttributes_PR_vehicle:
				this_lane_color = lane_color;
				break;
			case LaneTypeAttributes_PR_trackedVehicle:
				this_lane_color = tram_lane_color;
				break;
			case LaneTypeAttributes_PR_crosswalk:
				this_lane_color = pedestrian_lane_color;
				break;
			case LaneTypeAttributes_PR_bikeLane:
				this_lane_color = bike_lane_color;
				break;
			default:
				std::cout << "TODO: unknown lane type attribute: " << laneobj.attr.laneType.present << std::endl;
		}

		// TODO: lanesharing seems to be always zeroed out
		//printf("lane sharing buffer: [0]: 0x%02x  [1]: 0x%02x\n", laneobj.attr.sharedWith.buf[0], laneobj.attr.sharedWith.buf[1]);

		auto node = laneobj.nodes.begin();
		auto node_counter = 0;
		while (node != laneobj.nodes.end())
		{
			int64_t nx = node->x;
			int64_t ny = node->y;

			plx = lx;
			ply = ly;
			lx = nx;
			ly = ny;

			sf::Vector2<int64_t> start(plx, ply);
			coords = Utils::to_screen(lx, ly, refp.x, refp.y);
			lane_node_strip.append( sf::Vertex(coords, sf::Color::Cyan));

			coords = Utils::to_screen(node->lx, node->ly, refp.x, refp.y);
			lane.append(sf::Vertex(coords, this_lane_color));
			coords = Utils::to_screen(node->rx, node->ry, refp.x, refp.y);
			lane.append(sf::Vertex(coords, this_lane_color));

			if (node_counter == 0)
			{
				// skip drawing for the first node
			}
			else
			{
				if (node_counter == 1)
				{
					if (Utils::is_egress_lane(laneobj.attr.directionalUse))
					{
						auto astart = start + (node->dir * 150.0f);
						auto invdir = -node->dir;
						if (laneobj.egress_arrow == nullptr)
						{
							laneobj.egress_arrow = new sf::VertexArray(sf::Triangles, 3);
						}
						Utils::draw_arrow(dynamic_cast<sf::VertexArray *>(laneobj.egress_arrow), astart, invdir, refp);
					}
					if (Utils::is_ingress_lane(laneobj.attr.directionalUse))
					{
						auto astart = start + (node->dir * 100.0f);
						if (laneobj.ingress_arrow == nullptr)
						{
							laneobj.ingress_arrow = new sf::VertexArray(sf::Triangles, 3);
						}
						sf::Color color = sf::Color::Black;
						if (laneobj.can_turn_right)
						{
							color += sf::Color::Blue;
						}
						if (laneobj.can_turn_left)
						{
							color += sf::Color::Red;
						}
						if (laneobj.can_turn_straight)
						{
							color += sf::Color::Green;
						}
						Utils::draw_arrow(dynamic_cast<sf::VertexArray *>(laneobj.ingress_arrow), astart, node->dir, refp, color);
					}
				}
			}

			if (node->is(NodeAttributeXY_stopLine))
			{
				// TODO: draw a stopline
			}

			/*
			if (node->is(NodeAttributeXY_mergePoint))
			{
				auto astart = end + ndir * 100.0f / _main->get_scale();
				auto aend = ndir * 300.0f / _main->get_scale();
				//sf::VertexArray mp = Utils::draw_arrow(astart, aend, sf::Color::Blue);
				//laneobj.lane_markings.emplace_back(std::move(mp));
			}

			if (node->is(NodeAttributeXY_divergePoint))
			{
				auto astart = end + ndir * 100.0f / _main->get_scale();
				auto aend = ndir * 300.0f / _main->get_scale();
				//sf::VertexArray mp = Utils::draw_arrow(astart, aend, sf::Color::Red);
				//laneobj.lane_markings.emplace_back(std::move(mp));
			}
			//*/
			++node;
			++node_counter;
		}

		lane_geometries.emplace_back(std::move(lane));
		lane_outline_geometries.emplace_back(std::move(lane_outline));
		lane_nodes.emplace_back(std::move(lane_node_strip));

		// connections
		auto cit = laneobj.connections.begin();
		while (cit != laneobj.connections.end())
		{
			cit->build_geometry(standalone);

			++cit;
		}

		++lit;
	}
}

void IntersectionEntity::draw(sf::RenderTarget &target, sf::RenderStates states) const
{
	// Draw the RSU
	if (ref != nullptr)
	{
		target.draw(*ref, states);
	}

	/*
	std::for_each(lane_outline_geometries.begin(), lane_outline_geometries.end(), [&target, &states](const sf::VertexArray &va) {
		target.draw(va, states);
	});
	//*/
	std::for_each(lane_geometries.begin(), lane_geometries.end(), [&target, &states](const sf::VertexArray &va) {
		target.draw(va, states);
	});
	/*
	std::for_each(lane_nodes.begin(), lane_nodes.end(), [&target, &states](const sf::VertexArray &va) {
		target.draw(va, states);
	});
	//*/

	// connections

	std::for_each(lanes.begin(), lanes.end(), [&target, &states](const std::pair<const LaneID_t, Lane> &d) {
		auto &l = d.second;
		target.draw(l, states);
	});
}

void IntersectionEntity::add_connection(LaneID_t start, LaneID_t end, const SignalGroupID_t *sg)
{
	LaneConnection c;
	c.from = &lanes[start];
	c.to = &lanes[end];
	if (sg != nullptr)
	{
		c.signal_group = *sg;
	}
	else
	{
		c.signal_group = -1;
	}
	lanes[start].connections.emplace_back(std::move(c));
}

void IntersectionEntity::set_signal_group_state(SignalGroupID_t id, MovementPhaseState_t state)
{
	auto lit = lanes.begin();
	while (lit != lanes.end())
	{
		auto lcit = lit->second.connections.begin();
		while (lcit != lit->second.connections.end())
		{
			if (lcit->signal_group == id && lcit->va.getVertexCount() > 0)
			{
				lcit->set_state(state);
				// No return here as there might be multiple connections with the same signal group
			}
			++lcit;
		}
		++lit;
	}
}

void IntersectionEntity::set_signal_group_timing(SignalGroupID_t id, TimeMark_t min, TimeMark_t max, TimeMark_t likely, TimeIntervalConfidence_t conf)
{
	auto lit = lanes.begin();
	while (lit != lanes.end())
	{
		auto lcit = lit->second.connections.begin();
		while (lcit != lit->second.connections.end())
		{
			if (lcit->signal_group == id)
			{
				lcit->set_timing(min, max, likely, conf);
				// No return here as there might be multiple connections with the same signal group
			}
			++lcit;
		}
		++lit;
	}
}

void IntersectionEntity::add_lane_to_ingress_approach(ApproachID_t aid, LaneID_t lid)
{
	auto lane = &(lanes[lid]);
	ingress_approaches[aid].lanes.push_back(lane);
}

void IntersectionEntity::for_each_ingress_approach(std::function<void(Approach&)> callback)
{
	if (!ingress_approaches.empty())
	{
		for (auto &approach : ingress_approaches)
		{
			callback(approach.second);
		}
	}
}

void IntersectionEntity::infer_data()
{
	// figure out where you can go from a lane
	for(auto &p_lane : lanes)
	{
		auto &lane = p_lane.second;

		// calc the lane vertices
		auto lnit = lane.nodes.begin();
		while (lnit != lane.nodes.end())
		{
			if (lnit + 1 == lane.nodes.end())
			{
				// there is not a next node
				auto &node = *lnit;
				auto &prev = *(lnit - 1);

				node.dir = prev.dir;
				auto o = Utils::ortho(node.dir);
				auto off = o * (node.width / 2.0f);

				Utils::lat_lon_move(node.y, node.x, -off.x, -off.y, node.ly, node.lx);
				Utils::lat_lon_move(node.y, node.x, off.x, off.y, node.ry, node.rx);
			}
			else
			{
				// there is a next node
				auto &node = *lnit;
				auto &next = *(lnit + 1);

				auto lanedir = next.to_vec() - node.to_vec();
				node.dir = Utils::normalize(lanedir);

				auto o = Utils::ortho(node.dir);
				auto off = o * (node.width / 2.0f);

				Utils::lat_lon_move(node.y, node.x, -off.x, -off.y, node.ly, node.lx);
				Utils::lat_lon_move(node.y, node.x, off.x, off.y, node.ry, node.rx);
			}
			++lnit;
		}

		if (!lane.is_ingress)
		{
			continue;
		}
		// direction of the lane
		auto dir = lane.nodes[0].to_vec() - lane.nodes[1].to_vec();
		for (auto &con : lane.connections)
		{

			auto &olane = *con.to;
			auto odir = olane.nodes[1].to_vec() - olane.nodes[0].to_vec();

			auto ndir = Utils::normalize(dir);
			auto nodir = Utils::normalize(odir);

			auto angle = acos((float)(ndir.x*nodir.x+ndir.y*nodir.y)/(float)(Utils::length(ndir) * Utils::length(nodir)));
			auto vcross = ndir.x*nodir.y - ndir.y*nodir.x;
			auto deg_angle = angle * 180.0f / M_PI;

			if (deg_angle < 30.0f)
			{
				lane.can_turn_straight = true;
				con.turn_direction = TurnDirection::Straight;
			}
			else if (vcross < 0.0f)
			{
				lane.can_turn_right = true;
				con.turn_direction = TurnDirection::Right;
			}
			else if (vcross > 0.0f)
			{
				lane.can_turn_left = true;
				con.turn_direction = TurnDirection::Left;
			}
		}
	}
}

Lane &IntersectionEntity::get_lane(LaneID_t id)
{
	return lanes[id];
}

void LaneConnection::build_geometry(bool standalone)
{
// connections are always from one first node to another first node of a lane
	auto snode = from->nodes[0].to_vec();
	auto sdir = from->nodes[0].to_vec() - from->nodes[1].to_vec();
	auto nsdir = Utils::normalize(sdir);

	auto enode = to->nodes[0].to_vec();
	auto edir = to->nodes[0].to_vec() - to->nodes[1].to_vec();
	auto nedir = Utils::normalize(edir);

	auto cdir = to->nodes[0].to_vec() -  from->nodes[0].to_vec();
	auto start_offset = snode + nsdir * Utils::length(cdir) * BEZIER_CONTROL_LENGTH;
	auto end_offset = enode + nedir * Utils::length(cdir) * BEZIER_CONTROL_LENGTH;
	sf::Vector2f start, end, sc, ec;

	if (standalone)
	{
		start = Utils::to_screen(snode, from->intersection->ref_x, from->intersection->ref_y);
		end = Utils::to_screen(enode, to->intersection->ref_x, to->intersection->ref_y);
		sc = Utils::to_screen(start_offset, from->intersection->ref_x, from->intersection->ref_y);
		ec = Utils::to_screen(end_offset, to->intersection->ref_x, to->intersection->ref_y);
	}
	else
	{
		start = Utils::to_screen(snode);
		end = Utils::to_screen(enode);
		sc = Utils::to_screen(start_offset);
		ec = Utils::to_screen(end_offset);
	}

	va.setPrimitiveType(sf::LineStrip);
	Utils::bezier_to_va(start, end, sc, ec, BEZIER_SEGMENTS, va);

	for (size_t i = 0; i < va.getVertexCount(); ++i)
	{
		switch (state)
		{
			case MovementPhaseState_unavailable:
				va[i].color = sf::Color::White;
				break;
			case MovementPhaseState_stop_Then_Proceed:
			case MovementPhaseState_stop_And_Remain:
				va[i].color = sf::Color::Red;
				break;
			case MovementPhaseState_permissive_Movement_Allowed:
			case MovementPhaseState_protected_Movement_Allowed:
				va[i].color = sf::Color::Green;
				break;
			case MovementPhaseState_pre_Movement:
				va[i].color = i < va.getVertexCount()/2 ? sf::Color::Yellow : sf::Color::Red;
				break;
			case MovementPhaseState_permissive_clearance:
			case MovementPhaseState_protected_clearance:
				va[i].color = sf::Color::Yellow;
				break;
			case MovementPhaseState_dark:
				va[i].color = sf::Color(50, 50, 50);
				break;
			case MovementPhaseState_caution_Conflicting_Traffic:
				va[i].color = sf::Color(255, 165, 0);
				break;
			default:
				std::cout << "TODO: unhandled movement phase state " << state << std::endl;
		}
	}
}

void LaneConnection::draw(sf::RenderTarget &target, sf::RenderStates states) const
{
	target.draw(va, states);
}

void LaneConnection::set_state(MovementPhaseState_t newstate)
{
	state = newstate;
}

void LaneConnection::set_timing(TimeMark_t min, TimeMark_t max, TimeMark_t likely, TimeIntervalConfidence_t conf)
{
	min_end_time = min;
	max_end_time = max;
	likely_time = likely;
	confidence = conf;
}
