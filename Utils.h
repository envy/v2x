#ifndef V2X_UTILS_H
#define V2X_UTILS_H

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
	static bool is_ingress_lane(LaneDirection_t dir);
	static bool is_egress_lane(LaneDirection_t dir);
	static bool has_individual_vehicle_traffic(LaneSharing_t s);
	static bool has_tracked_vehicle_traffic(LaneSharing_t s);
	static bool has_bus_vehicle_traffic(LaneSharing_t s);
	static bool has_taxi_vehicle_traffic(LaneSharing_t s);
	static bool has_pedestrian_traffic(LaneSharing_t s);
	static bool has_cyclist_traffic(LaneSharing_t s);
	static MovementPhaseState_t get_movement_phase_for_signal_group(SPATEM_t *spatem, SignalGroupID_t &id);
	static sf::Vector2f ortho(sf::Vector2f &a);
	static sf::Vector2f normalize(sf::Vector2f &a);
	static float length(sf::Vector2f &a);
	static float timemark_to_seconds(TimeMark_t &t);
	static sf::Vector2<int32_t> lat_lon_to_x_y(double lat, double lon, uint8_t zoom);
	static void draw_arrow(sf::VertexArray *va, sf::Vector2f &start, sf::Vector2f &dir, sf::Color color = sf::Color::White);
};


#endif //V2X_UTILS_H
