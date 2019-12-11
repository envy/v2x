#ifndef V2X_INTERSECTIONENTITY_H
#define V2X_INTERSECTIONENTITY_H

#include <vector>
#include <map>
#include <tuple>
#include <cstdint>
#include <iostream>
#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include "NodeAttributeXY.h"
#include "LaneID.h"
#include "LaneAttributes.h"
#include "SignalGroupID.h"
#include "MovementPhaseState.h"

struct Node
{
public:
	int64_t x { 0 };
	int64_t y { 0 };
	uint64_t width { 0 };
	std::vector<NodeAttributeXY_t> attributes;
	bool is(NodeAttributeXY_t attr)
	{
		return std::find(attributes.begin(), attributes.end(), attr) != attributes.end();
	}
};

class LaneConnection : public sf::Drawable
{
public:
	LaneID_t to_id { 0 };
	SignalGroupID_t signal_group { 0 };
	sf::VertexArray va {};

	virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;
};

class Lane : public sf::Drawable
{
private:
public:
	~Lane() override;
	Lane() = default;
	Lane(Lane &&l) noexcept;
	Lane &operator=(Lane &&l) noexcept;
	LaneAttributes_t attr {};
	std::vector<Node> nodes;
	std::vector<LaneConnection> connections;
	sf::Drawable *ingress_arrow { new sf::VertexArray(sf::LineStrip, 5) };
	sf::Drawable *egress_arrow { new sf::VertexArray(sf::LineStrip, 5) };
	//std::vector<sf::VertexArray> lane_markings;

	virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;
};


class IntersectionEntity : public sf::Drawable
{
private:
	int64_t ref_x { 0 }, ref_y { 0 };
	std::map<LaneID_t, Lane> lanes;
	sf::Color lane_color { sf::Color(100, 100, 100) };
	sf::Color lane_outer_color { sf::Color(200, 200, 200) };
	sf::Color tram_lane_color { sf::Color(210, 180, 140) };
	sf::Color pedestrian_lane_color { sf::Color(135, 206, 235) };

	std::vector<sf::VertexArray> lane_geometries;
	std::vector<sf::VertexArray> lane_outline_geometries;
	std::vector<sf::VertexArray> lane_nodes;

	virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;

public:
	IntersectionEntity(int64_t ref_x, int64_t ref_y) : ref_x(ref_x), ref_y(ref_y) {}
	void build_geometry();
	void add_lane(LaneID_t id, LaneAttributes &attr);
	void add_node(LaneID_t lane_id, int64_t x, int64_t y, uint64_t width, std::vector<NodeAttributeXY_t> &attributes);
	void add_connection(LaneID_t start, LaneID_t end, const SignalGroupID_t *sg);
	void set_signal_group_state(SignalGroupID_t id, MovementPhaseState_t state);
};


#endif //V2X_INTERSECTIONENTITY_H
