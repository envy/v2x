#include "Utils.h"


bool Utils::is_ingress_lane(LaneDirection_t dir)
{
	return (dir.buf[0] & (1u << (7-LaneDirection_ingressPath))) > 0;
}

bool Utils::is_egress_lane(LaneDirection_t dir)
{
	return (dir.buf[0] & (1u << (7-LaneDirection_egressPath))) > 0;
}
