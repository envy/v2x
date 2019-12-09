#ifndef V2X_UTILS_H
#define V2X_UTILS_H

#include "MessageSink.h"

#include "LaneDirection.h"
#include "MovementPhaseState.h"
#include "SignalGroupID.h"

class Utils
{
private:
	Utils();

public:
	static bool is_ingress_lane(LaneDirection_t dir);
	static bool is_egress_lane(LaneDirection_t dir);
	static MovementPhaseState_t get_movement_phase_for_signal_group(SPATEM_t *spatem, SignalGroupID_t &id);
	static sf::Vector2f ortho(sf::Vector2f &a);
	static sf::Vector2f normalize(sf::Vector2f &a);
	static float length(sf::Vector2f &a);
	static sf::Vector2<int32_t> lat_lon_to_x_y(double lat, double lon, uint8_t zoom);
};


#endif //V2X_UTILS_H
