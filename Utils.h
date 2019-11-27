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
	static MovementPhaseState_t get_movement_phase_for_signal_group(station_msgs_t *data, SignalGroupID_t &id);
	static sf::Vector2f ortho(sf::Vector2f &a);
	static sf::Vector2f normalize(sf::Vector2f &a);
	static float length(sf::Vector2f &a);
};


#endif //V2X_UTILS_H
