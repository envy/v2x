#include "parser.h"

#include <iostream>
#include <sstream>
#include <iomanip>

#include "StationType.h"

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

std::string format_cam_station_type(StationType_t type)
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

int dump_packet(uint8_t *buf, uint32_t len)
{
	int32_t tlen = len;
	if (tlen < sizeof(ethernet_t))
	{
		std::cout << "Not enough data for ethernet header" << std::endl;
		return -1;
	}
	ethernet_t *e = (ethernet_t *)buf;
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

	geonetworking_t *g = (geonetworking_t *)e->data;
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

	btp_b_t *b = (btp_b_t *)(g->data + sizeof(geonet_shb_t));

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
	ethernet_t *e = (ethernet_t *)buf;
	geonetworking_t *g = (geonetworking_t *)e->data;
	switch (g->common_header.type.raw)
	{
		case GEONET_TYPE_SHB:
			return fix_offset + sizeof(geonet_shb_t);
		case GEONET_TYPE_BEACON:
			return fix_offset + sizeof(geonet_beacon_t);
		default:
			std::cerr << "FIXME: unknown geonet type 0x" << std::hex << (unsigned int)g->common_header.type.raw << std::dec << std::endl;
	}

	return fix_offset;
}

bool is_cam(uint8_t *buf, uint32_t len, uint32_t *cam_start)
{
	int btpoffset = btp_offset(buf, len);
	if (btpoffset <= 0)
	{
		return false;
	}

	ethernet_t *e = (ethernet_t *)buf;
	geonetworking_t *g = (geonetworking_t *)e->data;
	btp_b_t *b = (btp_b_t *)(buf + btpoffset);

	if (ntohs(b->port) == BTP_B_PORT_CAM)
	{
		if (cam_start)
		{
			*cam_start = (uint8_t *)b - buf + sizeof(btp_b_t);
		}
		return true;
	}

	return false;
}


int parse_cam(uint8_t *buf, uint32_t len, CAM_t **cam)
{


	asn_dec_rval_t ret;

	ret = uper_decode_complete(0, &asn_DEF_CAM, (void **)cam, buf, len);

	if (ret.code != RC_OK)
	{
		std::cout << "Could not decode CAM: ";
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
	int iret = asn_check_constraints(&asn_DEF_CAM, cam, errmsg, &errlen);
	if (iret != 0)
	{
		std::cout << "Contraint error: " << errmsg << std::endl;
		//return -2;
	}

	return 0;
}
