#ifndef V2X_INTERSECTIONENTITY_H
#define V2X_INTERSECTIONENTITY_H

#include <vector>
#include <map>
#include <tuple>
#include <cstdint>
#include <iostream>
#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include "asn_headers.h"

struct LaneNode
{
public:
	int64_t x { 0 };
	int64_t y { 0 };
	uint64_t width { 0 };
	int64_t lx { 0 };
	int64_t ly { 0 };
	int64_t rx { 0 };
	int64_t ry { 0 };
	std::vector<NodeAttributeXY_t> attributes;
	bool is(NodeAttributeXY_t attr)
	{
		return std::find(attributes.begin(), attributes.end(), attr) != attributes.end();
	}
	sf::Vector2<int64_t> to_vec()
	{
		return sf::Vector2<int64_t>(x, y);
	}
	sf::Vector2<int64_t> right_to_vec()
	{
		return sf::Vector2<int64_t>(rx, ry);
	}
	sf::Vector2<int64_t> left_to_vec()
	{
		return sf::Vector2<int64_t>(lx, ly);
	}

};

class Lane;

enum class TurnDirection
{
	Unknown,
	Straight,
	Left,
	Right,
};

class LaneConnection : public sf::Drawable
{
public:
	static constexpr uint32_t BEZIER_SEGMENTS = 20;
	static constexpr float BEZIER_CONTROL_LENGTH = 0.45f;
	LaneConnection() = default;
	Lane *to { nullptr };
	Lane *from { nullptr };
	SignalGroupID_t signal_group { 0 };
	MovementPhaseState_t state { MovementPhaseState_unavailable };
	sf::VertexArray va {};
	TurnDirection turn_direction { TurnDirection::Unknown };

	void build_geometry(bool standalone);
	void set_state(MovementPhaseState_t newstate);
	virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;
};

class IntersectionEntity;

class Lane : public sf::Drawable
{
private:
public:
	~Lane() override;
	Lane() = default;
	Lane(Lane &&l) noexcept;
	Lane &operator=(Lane &&l) noexcept;
	LaneID_t id { -1 };
	IntersectionEntity *intersection { nullptr };
	LaneAttributes_t attr {};
	std::vector<LaneNode> nodes;
	std::vector<LaneConnection> connections;
	sf::Drawable *ingress_arrow { nullptr };
	sf::Drawable *egress_arrow { nullptr };
	sf::Drawable *stopline { nullptr };

	bool is_ingress { false };
	bool is_egress { false };

	bool can_turn_left { false };
	bool can_turn_straight { false };
	bool can_turn_right { false };

	virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;
};

class Approach
{
public:
	std::vector<Lane *> lanes;
};

class IntersectionEntity : public sf::Drawable
{
	friend class Lane;
	friend class LaneConnection;
private:
	int64_t ref_x { 0 }, ref_y { 0 };
	std::map<ApproachID_t, Approach> ingress_approaches;
	std::map<LaneID_t, Lane> lanes;
	const sf::Color lane_color { sf::Color(100, 100, 100) };
	const sf::Color lane_outer_color { sf::Color(200, 200, 200) };
	const sf::Color tram_lane_color { sf::Color(210, 180, 140) };
	const sf::Color pedestrian_lane_color { sf::Color(135, 206, 235) };
	const sf::Color bike_lane_color { sf::Color(250, 128, 114) };

	sf::Drawable *ref { nullptr };
	std::vector<sf::VertexArray> lane_geometries;
	std::vector<sf::VertexArray> lane_outline_geometries;
	std::vector<sf::VertexArray> lane_nodes;

	virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;

public:
	IntersectionEntity(int64_t ref_x, int64_t ref_y) : ref_x(ref_x), ref_y(ref_y) {}
	sf::Vector2<int64_t> get_location();
	void build_geometry(bool standalone);
	void infer_data();
	void add_lane(LaneID_t id, LaneAttributes &attr);
	Lane &get_lane(LaneID_t id);
	void add_lane_to_ingress_approach(ApproachID_t aid, LaneID_t lid);
	void add_node(LaneID_t lane_id, int64_t x, int64_t y, uint64_t width, std::vector<NodeAttributeXY_t> &attributes);
	void add_connection(LaneID_t start, LaneID_t end, const SignalGroupID_t *sg);
	void set_signal_group_state(SignalGroupID_t id, MovementPhaseState_t state);
	void for_each_ingress_approach(std::function<void(Approach&)> callback);
};


#endif //V2X_INTERSECTIONENTITY_H
