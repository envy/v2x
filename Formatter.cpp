#include "Formatter.h"

#include <sstream>
#include <iomanip>
#include <arpa/inet.h>

#include "BasicVehicleContainerHighFrequency.h"
#include "BasicVehicleContainerHighFrequencyV1.h"
#include "LowFrequencyContainer.h"
#include "BasicVehicleContainerLowFrequency.h"
#include "RSUContainerHighFrequency.h"
#include "ProtectedCommunicationZonesRSU.h"
#include "ProtectedCommunicationZone.h"
#include "SituationContainer.h"
#include "SituationContainerV1.h"
#include "IntersectionState.h"
#include "RoadSegmentList.h"
#include "MovementState.h"
#include "MovementEvent.h"
#include "RoadSegment.h"
#include "GenericLane.h"
#include "ConnectsToList.h"
#include "Connection.h"
#include "IntersectionGeometryList.h"
#include "IntersectionGeometry.h"
#include "IntersectionReferenceID.h"
#include "IntersectionID.h"
#include "LaneList.h"
#include "TimeChangeDetails.h"
#include "Utils.h"

std::string Formatter::format_mac(uint8_t mac[6])
{
	std::stringstream ss;
	ss << std::setfill('0') << std::setw(2);
	ss << std::hex << (unsigned int)mac[0] << ":";
	ss << std::hex << (unsigned int)mac[1] << ":";
	ss << std::hex << (unsigned int)mac[2] << ":";
	ss << std::hex << (unsigned int)mac[3] << ":";
	ss << std::hex << (unsigned int)mac[4] << ":";
	ss << std::hex << (unsigned int)mac[5];
	return std::string(ss.str());
}

std::string Formatter::format_station_type(StationType_t type)
{
	switch (type)
	{
		case StationType_unknown:
			return "unknown";
		case StationType_pedestrian:
			return "Pedestrian";
		case StationType_cyclist:
			return "Cyclist";
		case StationType_moped:
			return "Moped";
		case StationType_motorcycle:
			return "Motercycle";
		case StationType_passengerCar:
			return "Car";
		case StationType_bus:
			return "Bus";
		case StationType_lightTruck:
			return "Light Truck";
		case StationType_heavyTruck:
			return "Heavy Truck";
		case StationType_trailer:
			return "Trailer";
		case StationType_specialVehicles:
			return "Special Vehicle";
		case StationType_tram:
			return "Tram";
		case StationType_roadSideUnit:
			return "RSU";
		default:
			std::stringstream ss;
			ss << "Unknown (" << type << ")";
			return ss.str();
	}
}

std::string Formatter::format_geonet_type(geonet_type_t type)
{
	switch (type)
	{
		case GEONET_TYPE_ANY:
			return "Any";
		case GEONET_TYPE_BEACON:
			return "Beacon";
		case GEONET_TYPE_GUC:
			return "Unicast";
		case GEONET_TYPE_GAC_CIRCLE:
			return "Anycast Circle";
		case GEONET_TYPE_GAC_RECT:
			return "Anycast Rectangle";
		case GEONET_TYPE_GAC_ELLIPSE:
			return "Anycast Ellipse";
		case GEONET_TYPE_GBC_CIRCLE:
			return "Broadcast Circle";
		case GEONET_TYPE_GBC_RECT:
			return "Broadcast Rectangle";
		case GEONET_TYPE_GBC_ELLIPSE:
			return "Broadcast Ellipse";
		case GEONET_TYPE_TSB_MHB:
			return "TSB MHB";
		case GEONET_TYPE_TSB_SHB:
			return "TSB SHB";
		case GEONET_TYPE_LS_REQUEST:
			return "LS Request";
		case GEONET_TYPE_LS_REPLY:
			return "LS Reply";
	}

	return "unknown";
}

std::string Formatter::format_vehicle_length(VehicleLengthValue_t val)
{
	std::stringstream ss;
	switch (val)
	{
		case VehicleLengthValue_unavailable:
			ss << "unavailable";
			break;
		case VehicleLengthValue_outOfRange:
			ss << "out of range";
			break;
		default:
			ss << val*10 << " cm";
	}
	return ss.str();
}

std::string Formatter::format_vehicle_width(VehicleWidth_t val)
{
	std::stringstream ss;
	switch (val)
	{
		case VehicleWidth_unavailable:
			ss << "unavailable";
			break;
		case VehicleWidth_outOfRange:
			ss << "out of range";
			break;
		default:
			ss << val*10 << " cm";
	}
	return ss.str();
}

std::string Formatter::format_vehicle_role(VehicleRole_t val)
{
	std::stringstream ss;
	switch (val)
	{
		case VehicleRole_publicTransport:
			ss << "Public Transport";
			break;
		case VehicleRole_specialTransport:
			ss << "Special Transport";
			break;
		case VehicleRole_dangerousGoods:
			ss << "Dangerous Goods";
			break;
		case VehicleRole_roadWork:
			ss << "Road Work";
			break;
		case VehicleRole_rescue:
			ss << "Rescue";
			break;
		case VehicleRole_emergency:
			ss << "Emergency";
			break;
		case VehicleRole_safetyCar:
			ss << "Safety Car";
			break;
		case VehicleRole_agriculture:
			ss << "Agriculture";
			break;
		case VehicleRole_commercial:
			ss << "Commercial";
			break;
		case VehicleRole_military:
			ss << "Military";
			break;
		case VehicleRole_roadOperator:
			ss << "Road Operator";
			break;
		case VehicleRole_taxi:
			ss << "Taxi";
			break;
		default:
			ss << "Unknown (" << val << ")";
	}
	return ss.str();
}

std::string Formatter::format_speed_value(SpeedValue_t val)
{
	std::stringstream ss;
	if (val == SpeedValue_unavailable) {
		return "unavailable";
	}
	ss << val << " cm/s (" << val / 100.0 << " m/s; " << (val/100.0) * 3.6 << " km/h)";
	return ss.str();
}

std::string Formatter::format_heading_value(HeadingValue_t val)
{
	std::stringstream ss;
	switch (val)
	{
		case HeadingValue_unavailable:
			ss << "unavailable";
			break;
		case HeadingValue_wgs84North:
			ss << "North";
			break;
		case HeadingValue_wgs84East:
			ss << "East";
			break;
		case HeadingValue_wgs84South:
			ss << "South";
			break;
		case HeadingValue_wgs84West:
			ss << "West";
			break;
		default:
			if (val > HeadingValue_wgs84North && val < HeadingValue_wgs84East)
			{
				ss << "North East " << "(" << val/10.0f << ")";
			}
			else if (val > HeadingValue_wgs84East && val < HeadingValue_wgs84South)
			{
				ss << "South East " << "(" << val/10.0f << ")";
			}
			else if (val > HeadingValue_wgs84South && val < HeadingValue_wgs84West)
			{
				ss << "South West " << "(" << val/10.0f << ")";
			}
			else if (val > HeadingValue_wgs84West && val < HeadingValue_unavailable)
			{
				ss << "North West " << "(" << val/10.0f << ")";
			}
	}
	return ss.str();
}

std::string Formatter::format_event_state(MovementPhaseState_t val)
{
	switch (val)
	{
		case MovementPhaseState_unavailable:
			return "unavailable";
		case MovementPhaseState_dark:
			return "dark";
		case MovementPhaseState_stop_Then_Proceed:
			return "STOP then proceed";
		case MovementPhaseState_stop_And_Remain:
			return "STOP and remain";
		case MovementPhaseState_pre_Movement:
			return "pre movement";
		case MovementPhaseState_permissive_Movement_Allowed:
			return "movement allowed (permissive)";
		case MovementPhaseState_protected_Movement_Allowed:
			return "movement allowed (protected)";
		case MovementPhaseState_permissive_clearance:
			return "clearance (permissive)";
		case MovementPhaseState_protected_clearance:
			return "clearance (protected)";
		case MovementPhaseState_caution_Conflicting_Traffic:
			return "/!\\ conflicting traffic";
		default:
			return "unknown";
	}
}

std::string Formatter::format_cause_code(CauseCode_t &val)
{
	switch (val.causeCode)
	{
		case CauseCodeType_reserved:
			return "reserved";
		case CauseCodeType_trafficCondition:
			return "Traffic condition";
		case CauseCodeType_accident:
			return "Accident";
		case CauseCodeType_roadworks:
			switch (val.subCauseCode)
			{
				default:
					return "Roadworks";
				case 1:
					return "??";
				case 2:
					return "Road marking work";
				case 3:
					return "??";
				case 4:
					return "Short-term stationary road works";
				case 5:
					return "Street cleaning";
				case 6:
					return "Winter service";
			}
		case CauseCodeType_impassability:
			return "Impassability";
		case CauseCodeType_adverseWeatherCondition_Adhesion:
			return "Adhesion";
		case CauseCodeType_aquaplaning:
			return "Aquaplaning";
		case CauseCodeType_hazardousLocation_SurfaceCondition:
			return "Surface condition";
		case CauseCodeType_hazardousLocation_ObstacleOnTheRoad:
			return "Obstacle on road";
		case CauseCodeType_hazardousLocation_AnimalOnTheRoad:
			return "Animal on the road";
		case CauseCodeType_humanPresenceOnTheRoad:
			return "Human on the road";
		case CauseCodeType_wrongWayDriving:
			return "Wrong way driving";
		case CauseCodeType_emergencyVehicleApproaching:
			return "Emergency vehicle approaching";
		default:
			return "unknown";
	}
}

std::string Formatter::format_cause_code(CauseCodeV1_t &val)
{
	CauseCode_t c {};
	c.causeCode = val.causeCode;
	c.subCauseCode = val.subCauseCode;
	return format_cause_code(c);
}

std::string Formatter::format_protected_zone_type(ProtectedZoneType_t val)
{
	switch (val)
	{
		case ProtectedZoneType_permanentCenDsrcTolling:
			return "permanent Cen Dsrc Tolling";
		case ProtectedZoneType_temporaryCenDsrcTolling:
			return "temporary Cen Dsrc Tolling";
		default:
			return "unknown";
	}
}

std::string Formatter::format_lane_direction(LaneDirection_t &val)
{
	std::stringstream ss;
	if (val.buf[0] & (1u << (7u-LaneDirection_egressPath)))
	{
		ss << "e";
	}
	else
	{
		ss << "-";
	}
	ss << "/";
	if (val.buf[0] & (1u << (7u-LaneDirection_ingressPath)))
	{
		ss << "i";
	}
	else
	{
		ss << "-";
	}
	return ss.str();
}

std::string Formatter::format_lane_type(LaneTypeAttributes_t &val)
{
	switch (val.present)
	{
		case LaneTypeAttributes_PR_vehicle:
			return "Vehicle";
		case LaneTypeAttributes_PR_crosswalk:
			return "Crosswalk";
		case LaneTypeAttributes_PR_bikeLane:
			return "Bike lane";
		case LaneTypeAttributes_PR_sidewalk:
			return "Sidewalk";
		case LaneTypeAttributes_PR_median:
			return "Median";
		case LaneTypeAttributes_PR_striping:
			return "Striping";
		case LaneTypeAttributes_PR_trackedVehicle:
			return "Tracked Vehicle";
		case LaneTypeAttributes_PR_parking:
			return "Parking";
		default:
			return "unknown";
	}
}

std::string Formatter::dump_camv1(CAMv1_t *cam)
{
	std::stringstream ss;
	ss << "CAM v" << cam->header.protocolVersion << " from ";
	ss << cam->header.stationID;
	ss << " (" << format_station_type(cam->cam.camParameters.basicContainer.stationType);
	ss << ") dT ";
	ss << cam->cam.generationDeltaTime;
	ss << std::endl;

	ss << " Location: ";
	if (cam->cam.camParameters.basicContainer.referencePosition.latitude == Latitude_unavailable)
	{
		ss << "lat. unavailable";
	}
	else
	{
		ss << cam->cam.camParameters.basicContainer.referencePosition.latitude / 10000000.0;
	}
	ss << ", ";
	if (cam->cam.camParameters.basicContainer.referencePosition.longitude == Longitude_unavailable)
	{
		ss << "lon. unavailable";
	}
	else
	{
		ss << cam->cam.camParameters.basicContainer.referencePosition.longitude / 10000000.0;
	}
	ss << std::endl;

	if (cam->cam.camParameters.highFrequencyContainer.present == HighFrequencyContainerV1_PR_basicVehicleContainerHighFrequency)
	{
		auto &b = cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency;
		//auto &b = cam->cam.camParameters.highFrequencyContainer.basicVehicleContainerHighFrequency;
		ss << " W x L: " << format_vehicle_width(b->vehicleWidth) << " x " << format_vehicle_length(b->vehicleLength.vehicleLengthValue) << std::endl;
		ss << " Speed: " << format_speed_value(b->speed.speedValue) << std::endl;
		ss << " Heading: " << format_heading_value(b->heading.headingValue) << std::endl;
	}
	else if (cam->cam.camParameters.highFrequencyContainer.present == HighFrequencyContainerV1_PR_rsuContainerHighFrequency)
	{
		auto &r = cam->cam.camParameters.highFrequencyContainer.choice.rsuContainerHighFrequency;
		//auto &r = cam->cam.camParameters.highFrequencyContainer.rsuContainerHighFrequency;
		if (r->protectedCommunicationZonesRSU != nullptr)
		{
			for(uint32_t i = 0; i < r->protectedCommunicationZonesRSU->list.count; ++i)
			{
				auto item = r->protectedCommunicationZonesRSU->list.array[i];
				double _lat = item->protectedZoneLatitude / 10000000.0;
				double _lon = item->protectedZoneLongitude / 10000000.0;
				ss << " Protected Zone: " << format_protected_zone_type(item->protectedZoneType) << std::endl;
				ss << " Location: " << _lat << ", " << _lon << std::endl;
			}
		}
	}

	if (cam->cam.camParameters.lowFrequencyContainer != nullptr)
	{
		auto lf = cam->cam.camParameters.lowFrequencyContainer;
		if (lf->present == LowFrequencyContainer_PR_basicVehicleContainerLowFrequency)
		{
			auto v = lf->choice.basicVehicleContainerLowFrequency;
			ss << " Vehicle Role: " << format_vehicle_role(v->vehicleRole) << std::endl;
		}
	}

	return ss.str();
}

std::string Formatter::dump_cam(CAM_t *cam)
{
	std::stringstream ss;
	ss << "CAM v" << cam->header.protocolVersion << " from ";
	ss << cam->header.stationID;
	ss << " (" << format_station_type(cam->cam.camParameters.basicContainer.stationType);
	ss << ") dT ";
	ss << cam->cam.generationDeltaTime;
	ss << std::endl;

	ss << " Location: ";
	double lat = cam->cam.camParameters.basicContainer.referencePosition.latitude / 10000000.0;
	double lon = cam->cam.camParameters.basicContainer.referencePosition.longitude / 10000000.0;
	ss << lat << ", " << lon;
	ss << std::endl;

	if (cam->cam.camParameters.highFrequencyContainer.present == HighFrequencyContainer_PR_basicVehicleContainerHighFrequency)
	{
		auto &b = cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency;
		//auto &b = cam->cam.camParameters.highFrequencyContainer.basicVehicleContainerHighFrequency;
		ss << " W x L: " << format_vehicle_width(b->vehicleWidth) << " x " << format_vehicle_length(b->vehicleLength.vehicleLengthValue) << std::endl;
		ss << " Speed: " << format_speed_value(b->speed.speedValue) << std::endl;
		ss << " Heading: " << format_heading_value(b->heading.headingValue) << std::endl;
	}
	else if (cam->cam.camParameters.highFrequencyContainer.present == HighFrequencyContainer_PR_rsuContainerHighFrequency)
	{
		auto &r = cam->cam.camParameters.highFrequencyContainer.choice.rsuContainerHighFrequency;
		//auto &r = cam->cam.camParameters.highFrequencyContainer.rsuContainerHighFrequency;
		if (r->protectedCommunicationZonesRSU != nullptr)
		{
			for(uint32_t i = 0; i < r->protectedCommunicationZonesRSU->list.count; ++i)
			{
				auto item = r->protectedCommunicationZonesRSU->list.array[i];
				double _lat = item->protectedZoneLatitude / 10000000.0;
				double _lon = item->protectedZoneLongitude / 10000000.0;
				ss << " Protected Zone: " << format_protected_zone_type(item->protectedZoneType) << std::endl;
				ss << " Location: " << _lat << ", " << _lon << std::endl;
			}
		}
	}

	return ss.str();
}

std::string Formatter::dump_denm(DENM_t *denm)
{
	std::stringstream ss;
	ss << "DENM v" << denm->header.protocolVersion << " from ";
	ss << denm->header.stationID;
	ss << std::endl;

	if (denm->denm.situation != nullptr)
	{
		auto situation = denm->denm.situation;
		ss << " Cause: " << format_cause_code(situation->eventType) << std::endl;
		ss << " Info Quality: " << situation->informationQuality << std::endl;
	}

	if (denm->denm.alacarte != nullptr)
	{
		auto alacarte = denm->denm.alacarte;
		if (alacarte->roadWorks != nullptr)
		{
			auto rw = alacarte->roadWorks;

		}
	}

	return ss.str();
}

std::string Formatter::dump_denmv1(DENMv1_t *denm)
{
	std::stringstream ss;
	ss << "DENM v" << denm->header.protocolVersion << " from ";
	ss << denm->header.stationID;
	ss << std::endl;

	if (denm->denm.situation != nullptr)
	{
		auto situation = denm->denm.situation;
		ss << " Cause: " << format_cause_code(situation->eventType) << std::endl;
		ss << " Info Quality: " << situation->informationQuality << std::endl;
	}

	return ss.str();
}

std::string Formatter::dump_spatem(SPATEM_t *spatem)
{
	std::stringstream ss;
	ss << "SPATEM v" << spatem->header.protocolVersion << " from ";
	ss << spatem->header.stationID;
	ss << std::endl;

	if (spatem->spat.timeStamp != nullptr)
	{
		ss << " Timestamp: " << *spatem->spat.timeStamp << std::endl;
	}

	for (uint32_t i = 0; i < spatem->spat.intersections.list.count; ++i)
	{
		auto intersection = spatem->spat.intersections.list.array[i];
		ss << " Intersection " << intersection->id.id << std::endl;
		for (uint32_t j = 0; j < intersection->states.list.count; ++j)
		{
			auto signal = intersection->states.list.array[j];
			ss << "  Signal Group " << signal->signalGroup << ": ";
			for (uint32_t k = 0; k < signal->state_time_speed.list.count; ++k)
			{
				if (k != 0)
				{
					ss << ", ";
				}
				auto state = signal->state_time_speed.list.array[k];
				ss << format_event_state(state->eventState);

				if (state->timing != nullptr)
				{
					ss.precision(0);
					ss << std::fixed;
					if (state->timing->likelyTime != nullptr)
					{
						ss << " ~" << Utils::timemark_to_seconds(*state->timing->likelyTime) << " s";
						ss << "  C: " << *state->timing->confidence;
					}
					else
					{
						ss << " >" << Utils::timemark_to_seconds(state->timing->minEndTime) << " s";
						if (state->timing->maxEndTime != nullptr)
						{
							ss << " <" << Utils::timemark_to_seconds(*state->timing->maxEndTime) << " s";
						}
					}
				}

				if (state->speeds != nullptr)
				{
					ss << "  TODO SPEEDS";
				}
			}
			ss << std::endl;
		}
	}

	return ss.str();
}

std::string Formatter::dump_mapem(MAPEM_t *mapem)
{
	std::stringstream ss;
	ss << "MAPEM v" << mapem->header.protocolVersion << " from ";
	ss << mapem->header.stationID;
	ss << std::endl;

	if (mapem->map.intersections != nullptr)
	{
		for (uint32_t _i = 0; _i < mapem->map.intersections->list.count; ++_i)
		{
			auto in = mapem->map.intersections->list.array[_i];
			ss << " Intersection: " << in->id.id;
			if (in->name != nullptr)
			{
				ss << " - " << std::string((char *)in->name->buf, in->name->size);
			}
			double lat = in->refPoint.lat / 10000000.0;
			double lon = in->refPoint.Long / 10000000.0;
			ss << " @ " << lat << ", " << lon << std::endl;
			for (uint32_t _l = 0; _l < in->laneSet.list.count; ++_l)
			{
				auto lane = in->laneSet.list.array[_l];
				ss << "  L " << lane->laneID << "(" << format_lane_type(lane->laneAttributes.laneType) << ") " << format_lane_direction(lane->laneAttributes.directionalUse);
				if (lane->connectsTo != nullptr)
				{
					ss << "   Connects to: ";
					for (uint32_t ct = 0; ct < lane->connectsTo->list.count; ++ct)
					{
						if (ct != 0)
						{
							ss << ", ";
						}
						auto con = lane->connectsTo->list.array[ct];
						ss << con->connectingLane.lane;
						if (con->signalGroup != nullptr)
						{
							ss << " (SG " << *con->signalGroup << ")";
						}
					}
					ss << std::endl;
				}
				else
				{
					ss << std::endl;
				}
			}
		}
	}

	if (mapem->map.roadSegments != nullptr)
	{
		for (uint32_t rs = 0; rs < mapem->map.roadSegments->list.count; ++rs)
		{
			auto seg = mapem->map.roadSegments->list.array[rs];
			ss << " Road Segment: " << seg->id.id;
			if (seg->name != nullptr)
			{
				ss << " - " << std::string((char *)seg->name->buf, seg->name->size);
			}
			ss << std::endl;
			double lat = seg->refPoint.lat / 10000000.0;
			double lon = seg->refPoint.Long / 10000000.0;
			ss << "  Location: " << lat << ", " << lon << std::endl;
			for (uint32_t ls = 0; ls < seg->roadLaneSet.list.count; ++ls)
			{
				auto rl = seg->roadLaneSet.list.array[ls];
				ss << "  Lane: " << rl->laneID << std::endl;
				ss << "   Direction: " << format_lane_direction(rl->laneAttributes.directionalUse) << std::endl;
				ss << "   Type: " << format_lane_type(rl->laneAttributes.laneType) << std::endl;
				if (rl->connectsTo != nullptr)
				{
					ss << "   Connects to: ";
					for (uint32_t ct = 0; ct < rl->connectsTo->list.count; ++ct)
					{
						if (ct != 0)
						{
							ss << ", ";
						}
						auto con = rl->connectsTo->list.array[ct];
						ss << con->connectingLane.lane;
						if (con->signalGroup != nullptr)
						{
							ss << " (SG " << *con->signalGroup << ")";
						}
					}
					ss << std::endl;
				}
			}
		}
	}

	return ss.str();
}
