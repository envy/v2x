#ifndef V2X_FORMATTER_H
#define V2X_FORMATTER_H

#include <string>
#include "parser.h"
#include "CAM.h"
#include "DENM.h"
#include "SPAT.h"
#include "MAPEM.h"

#include "VehicleLengthValue.h"
#include "VehicleWidth.h"
#include "VehicleRole.h"
#include "SpeedValue.h"
#include "HeadingValue.h"
#include "MovementPhaseState.h"
#include "CauseCode.h"
#include "ProtectedZoneType.h"
#include "LaneDirection.h"
#include "LaneTypeAttributes.h"

class Formatter
{
private:
public:
	Formatter() = delete;
	Formatter(Formatter &) = delete;
	Formatter(Formatter &&) = delete;
	static std::string format_mac(uint8_t mac[6]);
	static std::string format_station_type(StationType_t type);
	static std::string format_geonet_type(geonet_type_t type);
	static std::string format_vehicle_length(VehicleLengthValue_t val);
	static std::string format_vehicle_width(VehicleWidth_t val);
	static std::string format_vehicle_role(VehicleRole_t val);
	static std::string format_speed_value(SpeedValue_t val);
	static std::string format_heading_value(HeadingValue_t val);
	static std::string format_event_state(MovementPhaseState_t val);
	static std::string format_cause_code(CauseCode_t &val);
	static std::string format_protected_zone_type(ProtectedZoneType_t val);
	static std::string format_lane_direction(LaneDirection_t &val);
	static std::string format_lane_type(LaneTypeAttributes_t &val);

	static std::string dump_camv1(CAMv1_t *cam);
	static std::string dump_cam(CAM_t *cam);
	static std::string dump_denm(DENM_t *denm);
	static std::string dump_spatem(SPATEM_t *spatem);
	static std::string dump_mapem(MAPEM_t *mapem);
};


#endif //V2X_FORMATTER_H
