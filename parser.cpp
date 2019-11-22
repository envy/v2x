#include "parser.h"

#include <iostream>
#include <sstream>
#include <iomanip>

#define RED "\x1b[31m"
#define GREEN "\x1b[32m"
#define RST "\x1b[0m"

static std::string format_mac(uint8_t mac[6])
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

std::string format_station_type(StationType_t type)
{
	switch (type)
	{
		case StationType_pedestrian:
			return "Pedestrian";
		case StationType_roadSideUnit:
			return "RSU";
		case StationType_passengerCar:
			return "Car";
	}

	std::stringstream ss;
	ss << "Unknown (" << type << ")";
	return ss.str();
}

std::string format_geonet_type(geonet_type_t type)
{
	switch (type)
	{
		case GEONET_TYPE_SHB:
			return "SHB";
		case GEONET_TYPE_BEACON:
			return "Beacon";
	}

	return "unknown";
}

std::string format_vehicle_length(VehicleLengthValue_t val)
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

std::string format_vehicle_width(VehicleWidth_t val)
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

std::string format_speed_value(VehicleWidth_t val)
{
	std::stringstream ss;
	switch (val)
	{
		case SpeedValue_unavailable:
			ss << "unavailable";
			break;
		default:
			ss << val << " cm/s (" << val / 100 << " m/s; " << val * 3.6 << " km/h)";
	}
	return ss.str();
}

std::string format_heading_value(HeadingValue_t val)
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
				ss << "North East";
			}
			else if (val > HeadingValue_wgs84East && val < HeadingValue_wgs84South)
			{
				ss << "South East";
			}
			else if (val > HeadingValue_wgs84South && val < HeadingValue_wgs84West)
			{
				ss << "South West";
			}
			else if (val > HeadingValue_wgs84West && val < HeadingValue_unavailable)
			{
				ss << "North West";
			}
	}
	return ss.str();
}

std::string format_event_state(MovementPhaseState_t val)
{
	switch (val)
	{
		case MovementPhaseState_unavailable:
			return "unavailable";
		case MovementPhaseState_dark:
			return "dark";
		case MovementPhaseState_stop_Then_Proceed:
			return RED "STOP" RST " then proceed";
		case MovementPhaseState_stop_And_Remain:
			return RED "STOP" RST " and remain";
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

std::string format_cause_code(CauseCode_t val)
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
			return "Roadworks";
		case CauseCodeType_impassability:
			return "Impassability";
		case CauseCodeType_adverseWeatherCondition_Adhesion:
			return "Adhesion";
		case CauseCodeType_aquaplannning:
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

int dump_packet(uint8_t *buf, uint32_t len)
{
	int32_t tlen = len;
	if (tlen < sizeof(ethernet_t))
	{
		std::cout << "Not enough data for ethernet header" << std::endl;
		return -1;
	}
	auto *e = (ethernet_t *)buf;
	std::cout << "From: " << format_mac(e->source_mac) << std::endl;
	std::cout << "To:   " << format_mac(e->destination_mac) << std::endl;
	std::cout << "Type: 0x" << std::hex << ntohs(e->type) << std::dec << std::endl;

	if (ntohs(e->type) != ETHERTYPE_GEONET)
	{
		std::cout << "Next header is not a geonetworking header!" << std::endl;
		return -2;
	}

	tlen -= sizeof(ethernet_t);
	std::cout << tlen << " remaining" << std::endl;

	if (tlen < sizeof(geonetworking_t))
	{
		std::cout << "Not enough data for geonet header" << std::endl;
		return -3;
	}

	auto *g = (geonetworking_t *)e->data;
	std::cout << "Version: " << (unsigned int)g->basic_header.version << std::endl;
	std::cout << "Next Header: " << (unsigned int)g->basic_header.next_header << std::endl;
	std::cout << "Life Time: " << (unsigned int)g->basic_header.life_time << std::endl; // TODO: calc correct lifetime
	std::cout << "Remaining Hop Limit: " << (unsigned int)g->basic_header.remaining_hop_limit << std::endl;

	// TODO: check next header == common_header

	std::cout << "Next Header: " << (unsigned int)g->common_header.next_header << std::endl;
	std::cout << "Type: 0x" << std::hex << (unsigned int)g->common_header.type.raw << std::dec << std::endl;
	std::cout << "Traffic Class: " << (unsigned int)g->common_header.traffic_class << std::endl;
	std::cout << "Payload length: " << (unsigned int)ntohs(g->common_header.payload_length) << std::endl;
	std::cout << "Max Hop Limit: " << (unsigned int)g->common_header.max_hop_limit << std::endl;

	// TODO: check next header == btp-b

	tlen -= sizeof(geonetworking_t);
	std::cout << tlen << " remaining" << std::endl;

	if (tlen < sizeof(btp_b_t))
	{
		std::cout << "Not enough data for btp-b header" << std::endl;
		return -6;
	}

	auto *b = (btp_b_t *)(g->data + sizeof(geonet_shb_t));

	std::cout << "Port: " << (unsigned int)ntohs(b->port) << std::endl;
	std::cout << "Port Info: " << std::hex << (unsigned int)ntohs(b->port_info) << std::dec << std::endl;

	return 0;
}

int btp_offset(uint8_t *buf, uint32_t len)
{
	static const uint32_t fix_offset = sizeof(ethernet_t) + sizeof(geonetworking_t);
	if (len < fix_offset)
	{
		return -1;
	}
	auto *e = (ethernet_t *)buf;
	auto *g = (geonetworking_t *)e->data;
	switch (g->common_header.type.raw)
	{
		case GEONET_TYPE_SHB:
			return fix_offset + sizeof(geonet_shb_t);
		case GEONET_TYPE_BEACON:
			return fix_offset + sizeof(geonet_beacon_t);
		default:
			std::cerr << "FIXME: unknown geonet type 0x" << std::hex << (unsigned int)g->common_header.type.raw << std::dec << std::endl;
			return -1;
	}

	return fix_offset;
}

static bool is_something(uint8_t *buf, uint32_t len, uint32_t *start, uint16_t port)
{
	int btpoffset = btp_offset(buf, len);
	if (btpoffset <= 0 || btpoffset >= len)
	{
		return false;
	}

	auto *b = (btp_b_t *)(buf + btpoffset);

	if (ntohs(b->port) == port)
	{
		if (start)
		{
			*start = (uint8_t *)b - buf + sizeof(btp_b_t);
		}
		return true;
	}

	return false;
}

bool is_cam(uint8_t *buf, uint32_t len, uint32_t *cam_start)
{
	return is_something(buf, len, cam_start, BTP_B_PORT_CAM);
}

bool is_denm(uint8_t *buf, uint32_t len, uint32_t *denm_start)
{
	return is_something(buf, len, denm_start, BTP_B_PORT_DENM);
}

bool is_spat(uint8_t *buf, uint32_t len, uint32_t *spat_start)
{
	return is_something(buf, len, spat_start, BTP_B_PORT_SPAT);
}

static int parse_something(uint8_t *buf, uint32_t len, void **ptr, asn_TYPE_descriptor_t *desc)
{
	asn_dec_rval_t ret;

	ret = uper_decode_complete(nullptr, desc, ptr, buf, len);

	if (ret.code != RC_OK)
	{
		std::cout << "Could not decode: ";
		switch (ret.code)
		{
			case RC_WMORE:
				std::cout << "WMORE (not enought data)";
				break;
			case RC_FAIL:
				std::cout << "FAIL";
				break;
			default:
				std::cout << "??? (" << ret.code;
		}
		std::cout << std::endl;
		return -1;
	}

	char errmsg[256];
	size_t errlen = sizeof(errmsg);
	int iret = asn_check_constraints(desc, ptr, errmsg, &errlen);
	if (iret != 0)
	{
		//std::cout << "Constraint error: " << errmsg << std::endl;
		//return -2;
	}

	return 0;
}

int parse_cam(uint8_t *buf, uint32_t len, CAM_t **cam)
{
	return parse_something(buf, len, (void **)cam, &asn_DEF_CAM);
}

int parse_denm(uint8_t *buf, uint32_t len, DENM_t **denm)
{
	return parse_something(buf, len, (void **)denm, &asn_DEF_DENM);
}

int parse_spat(uint8_t *buf, uint32_t len, SPAT_t **spat)
{
	return parse_something(buf, len, (void **)spat, &asn_DEF_SPAT);
}
