#include <SFML/Graphics/Vertex.hpp>
#include "IntersectionEntity.h"
#include "main.h"
#include "Utils.h"

#include <iostream>
#include <cmath>

uint64_t IntersectionEntity::add_lane(uint64_t width)
{
	Lane l;
	l.width = width;
	lanes.emplace_back(std::move(l));
	return lanes.size() - 1;
}

void IntersectionEntity::add_node(uint64_t lane_id, int64_t x, int64_t y, bool is_stopline)
{
	Node n;
	n.x = x;
	n.y = y;
	n.is_stopline = is_stopline;
	lanes[lane_id].nodes.emplace_back(std::move(n));
}

void IntersectionEntity::build_geometry()
{
	lane_outline_geometries.clear();
	lane_geometries.clear();
	lane_markings.clear();
	lane_nodes.clear();

	auto lit = lanes.begin();
	while (lit != lanes.end())
	{
		auto laneobj = *lit;
		sf::VertexArray lane(sf::TrianglesStrip);
		sf::VertexArray lane_outline(sf::TrianglesStrip);
		sf::VertexArray lane_node_strip(sf::LineStrip);
		float prev_x, prev_y, x, y;

		float width = laneobj.width / _main->get_scale();
		float half_width = width / 2.0f;

		x = Main::get_center_x() + (ref_x - _main->get_origin_x()) / _main->get_scale();
		y = Main::get_center_y() - (ref_y - _main->get_origin_y()) / _main->get_scale();

		auto node = laneobj.nodes.begin();
		auto node_counter = 0;
		while (node != laneobj.nodes.end())
		{
			int64_t nx = node->x;
			int64_t ny = node->y;
			prev_x = x;
			prev_y = y;
			x = (float)(prev_x + (nx / _main->get_scale()));
			y = (float)(prev_y - (ny / _main->get_scale()));

			lane_node_strip.append( sf::Vertex(sf::Vector2f(x, y), sf::Color::Cyan));

			auto start = sf::Vector2f(prev_x, prev_y);
			auto end = sf::Vector2f(x, y);
			auto dir = end-start;
			auto off = Utils::ortho(dir) * half_width; // FIXME: this is not correct as 150cm does not equal the lat/long the vector is supposed to be doing here

			auto ostart = start - Utils::normalize(dir) * 1.1f;
			auto oend = end + Utils::normalize(dir) * 1.1f;
			auto ooff = Utils::ortho(dir) * half_width * 1.1f;

			if (node_counter == 0)
			{
				// skip drawing for the first node
			}
			else
			{
				lane.append(sf::Vertex(start - off, lane_color));
				lane.append(sf::Vertex(start + off, lane_color));

				lane_outline.append(sf::Vertex(ostart - ooff, lane_outer_color));
				lane_outline.append(sf::Vertex(ostart + ooff, lane_outer_color));

				if (node + 1 == laneobj.nodes.end())
				{
					lane.append(sf::Vertex(end - off, lane_color));
					lane.append(sf::Vertex(end + off, lane_color));

					lane_outline.append(sf::Vertex(oend - ooff, lane_outer_color));
					lane_outline.append(sf::Vertex(oend + ooff, lane_outer_color));
				}
			}

			if (node->is_stopline)
			{
				auto nextnode = node + 1;
				if (nextnode != laneobj.nodes.end())
				{
					auto local_prev_x = x;
					auto local_prev_y = y;
					auto local_x = (float)(local_prev_x + (nextnode->x / _main->get_scale()));
					auto local_y = (float)(local_prev_y - (nextnode->y / _main->get_scale()));
					auto local_start = sf::Vector2f(local_prev_x, local_prev_y);
					auto local_end = sf::Vector2f(local_x, local_y);
					auto local_dir = local_end - local_start;
					auto local_off = Utils::ortho(local_dir) * half_width; // FIXME: this is not correct as 150cm does not equal the lat/long the vector is supposed to be doing here
					sf::VertexArray stopline(sf::Quads);
					stopline.append(sf::Vertex(local_start - local_off, sf::Color::White));
					stopline.append(sf::Vertex(local_start + local_off, sf::Color::White));
					stopline.append(sf::Vertex((local_start + (Utils::normalize(dir) * 100.0f)) + local_off , sf::Color::White));
					stopline.append(sf::Vertex((local_start + (Utils::normalize(dir) * 100.0f)) - local_off, sf::Color::White));
					lane_markings.emplace_back(std::move(stopline));
				}

			}

			++node;
			++node_counter;
		}

		lane_geometries.emplace_back(std::move(lane));
		lane_outline_geometries.emplace_back(std::move(lane_outline));
		lane_nodes.emplace_back(std::move(lane_node_strip));

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
	std::for_each(lane_markings.begin(), lane_markings.end(), [&target](const sf::VertexArray &va) {
		target.draw(va);
	});
	std::for_each(lane_nodes.begin(), lane_nodes.end(), [&target](const sf::VertexArray &va) {
		target.draw(va);
	});
}
