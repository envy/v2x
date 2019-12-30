#ifndef V2X_UTILS_H
#define V2X_UTILS_H

#include <asn1-src/AllowedManeuvers.h>
#include "MessageSink.h"

#include "LaneDirection.h"
#include "MovementPhaseState.h"
#include "SignalGroupID.h"
#include "TimeMark.h"

class Utils
{
private:
	Utils();

public:
	static bool is_ingress_lane(LaneDirection_t &dir);
	static bool is_egress_lane(LaneDirection_t &dir);
	static bool has_individual_vehicle_traffic(LaneSharing_t s);
	static bool has_tracked_vehicle_traffic(LaneSharing_t s);
	static bool has_bus_vehicle_traffic(LaneSharing_t s);
	static bool has_taxi_vehicle_traffic(LaneSharing_t s);
	static bool has_pedestrian_traffic(LaneSharing_t s);
	static bool has_cyclist_traffic(LaneSharing_t s);
	static bool maneuver_right_turn_on_red(AllowedManeuvers_t &m);
	static MovementPhaseState_t get_movement_phase_for_signal_group(SPATEM_t *spatem, SignalGroupID_t &id);
	static sf::Vector2f ortho(sf::Vector2f &a);
	static sf::Vector2f ortho(sf::Vector2<int64_t> &a);
	static sf::Vector2f normalize(sf::Vector2f &a);
	static sf::Vector2f normalize(sf::Vector2<int64_t> &a);
	static sf::Vector2f rotate(sf::Vector2f &a, float angle);
	static float length(sf::Vector2f &a);
	static float length(sf::Vector2<int64_t> &a);
	static float timemark_to_seconds(TimeMark_t &t);
	static sf::Vector2<int32_t> lat_lon_to_x_y(double lat, double lon, uint8_t zoom);
	static void draw_arrow(sf::VertexArray *va, sf::Vector2f &start, sf::Vector2f &dir, sf::Vector2<int64_t> &ref, sf::Color color = sf::Color::White);
	static void draw_arrow(sf::VertexArray *va, sf::Vector2f &start, sf::Vector2f &dir, sf::Color color = sf::Color::White);
	static void bezier_to_va(const sf::Vector2f &start, const sf::Vector2f &end, const sf::Vector2f &startControl, const sf::Vector2f &endControl, const size_t numSegments, sf::VertexArray &va);
	static void lat_lon_move(int64_t lat, int64_t lon, int64_t x_cm, int64_t y_cm, int64_t &out_lat, int64_t &out_lon);
	static sf::Vector2f to_screen(sf::Vector2f &f);
	static sf::Vector2f to_screen(sf::Vector2f &f, int64_t ox, int64_t oy);
	static sf::Vector2f to_screen(sf::Vector2<int64_t> &i);
	static sf::Vector2f to_screen(sf::Vector2<int64_t> &i, int64_t ox, int64_t oy);
	static sf::Vector2f to_screen(int64_t ix, int64_t iy);
	static sf::Vector2f to_screen(int64_t ix, int64_t iy, int64_t ox, int64_t oy);
};

sf::Vector2<int64_t> operator*(sf::Vector2<int64_t> v, float a);
sf::Vector2<int64_t> operator/(sf::Vector2<int64_t> v, float a);
sf::Vector2f operator+(sf::Vector2f a, sf::Vector2<int64_t> b);
sf::Vector2f operator+(sf::Vector2<int64_t> a, sf::Vector2f b);

#endif //V2X_UTILS_H
