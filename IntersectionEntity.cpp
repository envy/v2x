#include <SFML/Graphics/Vertex.hpp>
#include "IntersectionEntity.h"
#include "main.h"
#include "Utils.h"

#include <iostream>
#include <cmath>

void IntersectionEntity::add_lane(LaneID_t id, LaneAttributes_t &attr)
{
	Lane l;
	l.attr = attr;
	lanes[id] = std::move(l);
}

void IntersectionEntity::add_node(LaneID_t lane_id, int64_t x, int64_t y, uint64_t width, std::vector<NodeAttributeXY_t> &attributes)
{
	auto &lane = lanes[lane_id];
	if (lane.nodes.empty())
	{
		//first node is referenced from ref-point
		Node n;
		n.x = ref_x + x;
		n.y = ref_y + y;
		n.width = width;
		n.attributes = std::move(attributes);
		lane.nodes.emplace_back(std::move(n));
	}
	else
	{
		//every other node is referenced from the previous node
		auto &prev_node = lane.nodes[lane.nodes.size() - 1];
		Node n;
		n.x = prev_node.x + x;
		n.y = prev_node.y + y;
		n.width = width;
		n.attributes = std::move(attributes);
		lane.nodes.emplace_back(std::move(n));
	}
}

void IntersectionEntity::build_geometry()
{
	if (_main == nullptr)
	{
		return;
	}
	lane_outline_geometries.clear();
	lane_geometries.clear();
	lane_markings.clear();
	lane_nodes.clear();

	auto lit = lanes.begin();
	while (lit != lanes.end())
	{
		auto &laneobj = lit->second;

		sf::VertexArray lane(sf::TrianglesStrip);
		sf::VertexArray lane_outline(sf::TrianglesStrip);
		sf::VertexArray lane_node_strip(sf::LineStrip);
		float prev_x, prev_y, x, y;

		x = Main::get_center_x() + (ref_x - _main->get_origin_x()) / _main->get_scale();
		y = Main::get_center_y() - (ref_y - _main->get_origin_y()) / _main->get_scale();

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
				this_lane_color = pedestrian_lane_color; //FIXME: own color for bike lane
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
			float width = (node->width) / _main->get_scale();
			float half_width = width / 2.0f;
			int64_t nx = node->x;
			int64_t ny = node->y;
			prev_x = x;
			prev_y = y;
			x = Main::get_center_x() + (nx - _main->get_origin_x()) / _main->get_scale();
			y = Main::get_center_y() - (ny - _main->get_origin_y()) / _main->get_scale();

			lane_node_strip.append( sf::Vertex(sf::Vector2f(x, y), sf::Color::Cyan));

			auto start = sf::Vector2f(prev_x, prev_y);
			auto end = sf::Vector2f(x, y);
			auto dir = end-start;
			auto ndir = Utils::normalize(dir);
			auto off = Utils::ortho(dir) * half_width; // FIXME: this is not correct as 150cm does not equal the lat/long the vector is supposed to be doing here

			auto ostart = start - ndir * 1.1f;
			auto oend = end + ndir * 1.1f;
			auto ooff = Utils::ortho(dir) * half_width * 1.1f;

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
						auto astart = start + ndir * 100.0f / _main->get_scale();
						auto aend = ndir * 300.0f / _main->get_scale();
						auto s = astart + aend;
						auto e = -aend;
						sf::VertexArray va = Utils::draw_arrow(s, e);
						lane_markings.emplace_back(std::move(va));
					}
					if (Utils::is_ingress_lane(laneobj.attr.directionalUse))
					{
						auto astart = start + ndir * 100.0f / _main->get_scale();
						auto aend = ndir * 300.0f / _main->get_scale();
						sf::VertexArray va = Utils::draw_arrow(astart, aend);
						lane_markings.emplace_back(std::move(va));
					}
				}

				lane.append(sf::Vertex(start - off, this_lane_color));
				lane.append(sf::Vertex(start + off, this_lane_color));

				lane_outline.append(sf::Vertex(ostart - ooff, lane_outer_color));
				lane_outline.append(sf::Vertex(ostart + ooff, lane_outer_color));

				if (node + 1 == laneobj.nodes.end())
				{
					lane.append(sf::Vertex(end - off, this_lane_color));
					lane.append(sf::Vertex(end + off, this_lane_color));

					lane_outline.append(sf::Vertex(oend - ooff, lane_outer_color));
					lane_outline.append(sf::Vertex(oend + ooff, lane_outer_color));
				}
			}

			if (node->is(NodeAttributeXY_stopLine))
			{
				auto nextnode = node + 1;
				if (nextnode != laneobj.nodes.end())
				{
					auto local_prev_x = x;
					auto local_prev_y = y;
					auto local_x = Main::get_center_x() + (nextnode->x - _main->get_origin_x()) / _main->get_scale();
					auto local_y = Main::get_center_y() - (nextnode->y - _main->get_origin_y()) / _main->get_scale();
					auto local_start = sf::Vector2f(local_prev_x, local_prev_y);
					auto local_end = sf::Vector2f(local_x, local_y);
					auto local_dir = local_end - local_start;
					auto local_off = Utils::ortho(local_dir) * half_width; // FIXME: this is not correct as 150cm does not equal the lat/long the vector is supposed to be doing here
					local_end = (local_start + Utils::normalize(local_dir) * 30.0f/_main->get_scale());
					sf::VertexArray stopline(sf::Quads);
					stopline.append(sf::Vertex(local_start - local_off, sf::Color::White));
					stopline.append(sf::Vertex(local_start + local_off, sf::Color::White));
					stopline.append(sf::Vertex(local_end + local_off , sf::Color::White));
					stopline.append(sf::Vertex(local_end - local_off, sf::Color::White));
					lane_markings.emplace_back(std::move(stopline));
				}
			}

			if (node->is(NodeAttributeXY_mergePoint))
			{
				auto astart = end + ndir * 100.0f / _main->get_scale();
				auto aend = ndir * 300.0f / _main->get_scale();
				sf::VertexArray mp = Utils::draw_arrow(astart, aend, sf::Color::Blue);
				lane_markings.emplace_back(std::move(mp));
			}

			if (node->is(NodeAttributeXY_divergePoint))
			{
				auto astart = end + ndir * 100.0f / _main->get_scale();
				auto aend = ndir * 300.0f / _main->get_scale();
				sf::VertexArray mp = Utils::draw_arrow(astart, aend, sf::Color::Red);
				lane_markings.emplace_back(std::move(mp));
			}

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
			auto &firstnode = laneobj.nodes[0];
			auto &othernode = lanes[cit->to_id].nodes[0];
			auto first_x = Main::get_center_x() + (firstnode.x - _main->get_origin_x()) / _main->get_scale();
			auto first_y = Main::get_center_y() - (firstnode.y - _main->get_origin_y()) / _main->get_scale();
			auto first = sf::Vector2f(first_x, first_y);
			auto other_x = Main::get_center_x() + (othernode.x - _main->get_origin_x()) / _main->get_scale();
			auto other_y = Main::get_center_y() - (othernode.y - _main->get_origin_y()) / _main->get_scale();
			auto other = sf::Vector2f(other_x, other_y);
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
	std::for_each(lane_outline_geometries.begin(), lane_outline_geometries.end(), [&target](const sf::VertexArray &va) {
		target.draw(va);
	});
	std::for_each(lane_geometries.begin(), lane_geometries.end(), [&target](const sf::VertexArray &va) {
		target.draw(va);
	});
	/*
	std::for_each(lane_nodes.begin(), lane_nodes.end(), [&target](const sf::VertexArray &va) {
		target.draw(va);
	});
	//*/
	std::for_each(lane_markings.begin(), lane_markings.end(), [&target](const sf::VertexArray &va) {
		target.draw(va);
	});

	// connections
	std::for_each(lanes.begin(), lanes.end(), [&target](const std::pair<const LaneID_t, Lane> &d) {
		auto &l = d.second;
		std::for_each(l.connections.begin(), l.connections.end(), [&target](const LaneConnection &lc) {
			target.draw(lc);
		});
	});
}

void IntersectionEntity::add_connection(LaneID_t start, LaneID_t end, const SignalGroupID_t *sg)
{
	LaneConnection c;
	c.to_id = end;
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
				switch (state)
				{
					case MovementPhaseState_stop_Then_Proceed:
					case MovementPhaseState_stop_And_Remain:
						lcit->va[0].color = sf::Color::Red;
						lcit->va[1].color = sf::Color::Red;
						break;
					case MovementPhaseState_permissive_Movement_Allowed:
					case MovementPhaseState_protected_Movement_Allowed:
						lcit->va[0].color = sf::Color::Green;
						lcit->va[1].color = sf::Color::Green;
						break;
					case MovementPhaseState_pre_Movement:
						lcit->va[0].color = sf::Color::Yellow;
						lcit->va[1].color = sf::Color::Red;
						break;
					case MovementPhaseState_permissive_clearance:
					case MovementPhaseState_protected_clearance:
						lcit->va[0].color = sf::Color::Yellow;
						lcit->va[1].color = sf::Color::Yellow;
						break;
					case MovementPhaseState_dark:
						lcit->va[0].color = sf::Color(50, 50, 50);
						lcit->va[1].color = sf::Color(50, 50, 50);
						break;
					case MovementPhaseState_caution_Conflicting_Traffic:
						lcit->va[0].color = sf::Color(255, 165, 0);
						lcit->va[1].color = sf::Color(255, 165, 0);
						break;
					default:
						std::cout << "TODO: unhandled movement phase state " << state << std::endl;
				}
				// No return here as there might be multiple connections with the smae signal group
			}
			++lcit;
		}
		++lit;
	}
}

void LaneConnection::draw(sf::RenderTarget &target, sf::RenderStates states) const
{
	target.draw(va);
}

void Lane::draw(sf::RenderTarget &target, sf::RenderStates states) const
{

}
