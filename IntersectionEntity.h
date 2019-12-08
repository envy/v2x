#ifndef V2X_INTERSECTIONENTITY_H
#define V2X_INTERSECTIONENTITY_H

#include <vector>
#include <tuple>
#include <cstdint>
#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include "NodeAttributeXY.h"

struct Node
{
public:
	int64_t x { 0 };
	int64_t y { 0 };
	bool is_stopline { false };
};

class Lane
{
private:
public:
	uint64_t width;
	std::vector<Node> nodes;
};


class IntersectionEntity : public sf::Drawable
{
private:
	int64_t ref_x, ref_y;
	std::vector<Lane> lanes;
	sf::Color lane_color = sf::Color(100, 100, 100);
	sf::Color lane_outer_color = sf::Color(50, 50, 50);

	std::vector<sf::VertexArray> lane_geometries;
	std::vector<sf::VertexArray> lane_outline_geometries;
	std::vector<sf::VertexArray> lane_markings;
	std::vector<sf::VertexArray> lane_nodes;

	virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;

public:
	IntersectionEntity(int64_t ref_x, int64_t ref_y) : ref_x(ref_x), ref_y(ref_y) {}
	void build_geometry();
	uint64_t add_lane(uint64_t width);
	void add_node(uint64_t lane_id, int64_t x, int64_t y, bool is_stopline);
};


#endif //V2X_INTERSECTIONENTITY_H
