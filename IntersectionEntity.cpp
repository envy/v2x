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
	attr = l.attr;
	l.attr.directionalUse.buf = nullptr;
	ingress_arrow = l.ingress_arrow;
	l.ingress_arrow = nullptr;
	egress_arrow = l.egress_arrow;
	l.egress_arrow = nullptr;
	connections = std::move(l.connections);
	nodes = std::move(l.nodes);
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

	attr = l.attr;
	l.attr.directionalUse.buf = nullptr;
	ingress_arrow = l.ingress_arrow;
	l.ingress_arrow = nullptr;
	egress_arrow = l.egress_arrow;
	l.egress_arrow = nullptr;
	connections = std::move(l.connections);
	nodes = std::move(l.nodes);

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
		Node n;
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
		Node n;
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

	// RSU
	auto r = 100/_main->get_scale();
	if (ref == nullptr)
	{
		ref = new sf::CircleShape(r);
		dynamic_cast<sf::CircleShape *>(ref)->setOrigin(r, r);
		dynamic_cast<sf::CircleShape *>(ref)->setFillColor(sf::Color::Cyan);
	}

	if (standalone)
	{
		dynamic_cast<sf::CircleShape *>(ref)->setPosition(Utils::to_screen(ref_x, ref_y, ref_x, ref_y));
	}
	else
	{
		dynamic_cast<sf::CircleShape *>(ref)->setPosition(Utils::to_screen(ref_x, ref_y));
	}

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
		auto ref = sf::Vector2<int64_t>(ref_x, ref_y);

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
			float width = node->width;
			float half_width = width / 2.0f;
			int64_t nx = node->x;
			int64_t ny = node->y;

			plx = lx;
			ply = ly;
			lx = nx;
			ly = ny;

			sf::Vector2<int64_t> start(plx, ply);
			sf::Vector2<int64_t> end(lx, ly);
			auto dir = end - start;
			auto ndir = Utils::normalize(dir);
			auto ortho = Utils::ortho(dir);
			auto off = ortho * half_width;

			if (standalone)
			{
				coords = Utils::to_screen(lx, ly, ref_x, ref_y);
			}
			else
			{
				coords = Utils::to_screen(lx, ly);
			}
			lane_node_strip.append( sf::Vertex(coords, sf::Color::Cyan));

			/*
			auto ostart = start - ndir * 1.1f;
			auto oend = end + ndir * 1.1f;
			auto ooff = Utils::ortho(dir) * half_width * 1.1f;
			//*/

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
						auto astart = start + (ndir * 150.0f);
						auto invdir = -ndir;
						if (laneobj.egress_arrow == nullptr)
						{
							laneobj.egress_arrow = new sf::VertexArray(sf::Triangles, 3);
						}
						if (standalone)
						{
							Utils::draw_arrow(dynamic_cast<sf::VertexArray *>(laneobj.egress_arrow), astart, invdir, ref);
						}
						else
						{
							Utils::draw_arrow(dynamic_cast<sf::VertexArray *>(laneobj.egress_arrow), astart, invdir);
						}
					}
					if (Utils::is_ingress_lane(laneobj.attr.directionalUse))
					{
						auto astart = start + (ndir * 100.0f);
						if (laneobj.ingress_arrow == nullptr)
						{
							laneobj.ingress_arrow = new sf::VertexArray(sf::Triangles, 3);
						}
						if (standalone)
						{
							Utils::draw_arrow(dynamic_cast<sf::VertexArray *>(laneobj.ingress_arrow), astart, ndir, ref);
						}
						else
						{
							Utils::draw_arrow(dynamic_cast<sf::VertexArray *>(laneobj.ingress_arrow), astart, ndir);
						}
					}
				}

				int64_t left_x, left_y, right_x, right_y;
				Utils::lat_lon_move(start.y, start.x, -off.x, -off.y, left_y, left_x);
				if (standalone)
				{
					coords = Utils::to_screen(left_x, left_y, ref_x, ref_y);
				}
				else
				{
					coords = Utils::to_screen(left_x, left_y);
				}
				lane.append(sf::Vertex(coords, this_lane_color));

				Utils::lat_lon_move(start.y, start.x, off.x, off.y, right_y, right_x);
				if (standalone)
				{
					coords = Utils::to_screen(right_x, right_y, ref_x, ref_y);
				}
				else
				{
					coords = Utils::to_screen(right_x, right_y);
				}
				lane.append(sf::Vertex(coords, this_lane_color));

				//lane_outline.append(sf::Vertex(ostart - ooff, lane_outer_color));
				//lane_outline.append(sf::Vertex(ostart + ooff, lane_outer_color));

				if (node + 1 == laneobj.nodes.end())
				{
					Utils::lat_lon_move(end.y, end.x, -off.x, -off.y, left_y, left_x);
					if (standalone)
					{
						coords = Utils::to_screen(left_x, left_y, ref_x, ref_y);
					}
					else
					{
						coords = Utils::to_screen(left_x, left_y);
					}
					lane.append(sf::Vertex(coords, this_lane_color));

					Utils::lat_lon_move(end.y, end.x, off.x, off.y, right_y, right_x);
					if (standalone)
					{
						coords = Utils::to_screen(right_x, right_y, ref_x, ref_y);
					}
					else
					{
						coords = Utils::to_screen(right_x, right_y);
					}
					lane.append(sf::Vertex(coords, this_lane_color));

					//lane_outline.append(sf::Vertex(oend - ooff, lane_outer_color));
					//lane_outline.append(sf::Vertex(oend + ooff, lane_outer_color));
				}
			}

			if (node->is(NodeAttributeXY_stopLine))
			{
				auto nextnode = node + 1;
				if (nextnode != laneobj.nodes.end())
				{
					/*
					auto local_prev_x = x;
					auto local_prev_y = y;
					auto local_x = Main::get_center_x() + (nextnode->x - _main->get_origin_x()) / _main->get_scale();
					auto local_y = Main::get_center_y() - (nextnode->y - _main->get_origin_y()) / _main->get_scale();
					auto local_start = sf::Vector2f(local_prev_x, local_prev_y);
					auto local_end = sf::Vector2f(local_x, local_y);
					auto local_dir = local_end - local_start;
					auto local_off = Utils::ortho(local_dir) * half_width; // FIXME: this is not correct as 150cm does not equal the lat/long the vector is supposed to be doing here
					local_end = (local_start + Utils::normalize(local_dir) * 30.0f/_main->get_scale());
					if (laneobj.stopline == nullptr)
					{
						laneobj.stopline = new sf::VertexArray(sf::Quads, 4);
						(*dynamic_cast<sf::VertexArray *>(laneobj.stopline))[0].color = sf::Color::White;
						(*dynamic_cast<sf::VertexArray *>(laneobj.stopline))[1].color = sf::Color::White;
						(*dynamic_cast<sf::VertexArray *>(laneobj.stopline))[2].color = sf::Color::White;
						(*dynamic_cast<sf::VertexArray *>(laneobj.stopline))[3].color = sf::Color::White;
					}
					(*dynamic_cast<sf::VertexArray *>(laneobj.stopline))[0].position = local_start - local_off;
					(*dynamic_cast<sf::VertexArray *>(laneobj.stopline))[1].position = local_start + local_off;
					(*dynamic_cast<sf::VertexArray *>(laneobj.stopline))[2].position = local_end + local_off;
					(*dynamic_cast<sf::VertexArray *>(laneobj.stopline))[3].position = local_end - local_off;
					//*/
				}
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
			// connections are always from one first node to another first node of a lane
			if (laneobj.nodes.empty())
			{
				++cit;
				continue;
			}
			auto firstnode = laneobj.nodes[0].to_vec();
			auto othernode = lanes[cit->to_id].nodes[0].to_vec();
			sf::Vector2f first, other;

			if (standalone)
			{
				first = Utils::to_screen(firstnode, ref_x, ref_y);
				other = Utils::to_screen(othernode, ref_x, ref_y);
			}
			else
			{
				first = Utils::to_screen(firstnode);
				other = Utils::to_screen(othernode);
			}

			if (cit->va.getVertexCount() == 0)
			{
				cit->va.setPrimitiveType(sf::LineStrip);
				cit->va.append(sf::Vertex(first, sf::Color::White));
				cit->va.append(sf::Vertex(other, sf::Color::White));
			}
			else
			{
				cit->va[0].position = first;
				cit->va[1].position = other;
			}

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
	c.from_id = start;
	c.to_id = end;
	if (sg != nullptr)
	{
		c.signal_group = *sg;
	}
	else
	{
		c.signal_group = -1;
	}
	c.from_node = &lanes[start].nodes[0];
	c.to_node = &lanes[end].nodes[1];
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

void LaneConnection::draw(sf::RenderTarget &target, sf::RenderStates states) const
{
	target.draw(va, states);
}

void LaneConnection::set_state(MovementPhaseState_t newstate)
{
	if (newstate == state)
	{
		return;
	}

	switch (newstate)
	{
		case MovementPhaseState_unavailable:
			va[0].color = sf::Color::Black;
			va[1].color = sf::Color::Black;
		case MovementPhaseState_stop_Then_Proceed:
		case MovementPhaseState_stop_And_Remain:
			va[0].color = sf::Color::Red;
			va[1].color = sf::Color::Red;
			break;
		case MovementPhaseState_permissive_Movement_Allowed:
		case MovementPhaseState_protected_Movement_Allowed:
			va[0].color = sf::Color::Green;
			va[1].color = sf::Color::Green;
			break;
		case MovementPhaseState_pre_Movement:
			va[0].color = sf::Color::Yellow;
			va[1].color = sf::Color::Red;
			break;
		case MovementPhaseState_permissive_clearance:
		case MovementPhaseState_protected_clearance:
			va[0].color = sf::Color::Yellow;
			va[1].color = sf::Color::Yellow;
			break;
		case MovementPhaseState_dark:
			va[0].color = sf::Color(50, 50, 50);
			va[1].color = sf::Color(50, 50, 50);
			break;
		case MovementPhaseState_caution_Conflicting_Traffic:
			va[0].color = sf::Color(255, 165, 0);
			va[1].color = sf::Color(255, 165, 0);
			break;
		default:
			std::cout << "TODO: unhandled movement phase state " << state << std::endl;
	}
}
