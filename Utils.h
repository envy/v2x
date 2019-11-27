#ifndef V2X_UTILS_H
#define V2X_UTILS_H

#include "LaneDirection.h"

class Utils
{
private:
	Utils();

public:
	static bool is_ingress_lane(LaneDirection_t dir);
	static bool is_egress_lane(LaneDirection_t dir);
};


#endif //V2X_UTILS_H
