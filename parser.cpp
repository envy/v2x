#include "parser.h"
#include "Formatter.h"

#include <iostream>

int dump_packet(uint8_t *buf, uint32_t len)
{
	int32_t tlen = len;
	if (tlen < sizeof(ethernet_t))
	{
		std::cout << "Not enough data for ethernet header" << std::endl;
		return -1;
	}
	auto *e = (ethernet_t *)buf;
	std::cout << "From: " << Formatter::format_mac(e->source_mac) << std::endl;
	std::cout << "To:   " << Formatter::format_mac(e->destination_mac) << std::endl;
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
	std::cout << "Life Time: " << (unsigned int)g->basic_header.life_time.raw << std::endl; // TODO: calc correct lifetime
	std::cout << "Remaining Hop Limit: " << (unsigned int)g->basic_header.remaining_hop_limit << std::endl;

	// TODO: check next header == common_header

	std::cout << "Next Header: " << (unsigned int)g->common_header.next_header << std::endl;
	std::cout << "Type: 0x" << std::hex << (unsigned int)g->common_header.type.raw << std::dec << std::endl;
	std::cout << "Traffic Class: " << (unsigned int)g->common_header.traffic_class.raw << std::endl;
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

	auto *b = (btp_b_t *)(g->data + sizeof(geonet_tsb_shb_t));

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
		case GEONET_TYPE_GAC_CIRCLE:
		case GEONET_TYPE_GAC_ELLIPSE:
		case GEONET_TYPE_GAC_RECT:
		case GEONET_TYPE_GBC_CIRCLE:
		case GEONET_TYPE_GBC_ELLIPSE:
		case GEONET_TYPE_GBC_RECT:
			return fix_offset + sizeof(geonet_gac_t);
		case GEONET_TYPE_GUC:
			return fix_offset + sizeof(geonet_guc_t);
		case GEONET_TYPE_TSB_MHB:
			return fix_offset + sizeof(geonet_tsb_mhb_t);
		case GEONET_TYPE_TSB_SHB:
			return fix_offset + sizeof(geonet_tsb_shb_t);
		case GEONET_TYPE_BEACON:
			return fix_offset + sizeof(geonet_beacon_t);
		default:
			std::cerr << "FIXME: unknown geonet type 0x" << std::hex << (unsigned int)g->common_header.type.raw << std::dec << std::endl;
			return -1;
	}
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

bool is_mapem(uint8_t *buf, uint32_t len, uint32_t *mapem_start)
{
	return is_something(buf, len, mapem_start, BTP_B_PORT_MAPEM);
}

bool is_spatem(uint8_t *buf, uint32_t len, uint32_t *spatem_start)
{
	return is_something(buf, len, spatem_start, BTP_B_PORT_SPATEM);
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
				std::cout << "WMORE (not enough data, len: " << len << ")";
				break;
			case RC_FAIL:
				std::cout << "FAIL";
				break;
			default:
				std::cout << "??? (" << ret.code << ")";
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

int parse_header(uint8_t *buf, uint32_t len, ItsPduHeader_t **header)
{
	return parse_something(buf, len, (void **)header, &asn_DEF_ItsPduHeader);
}

int parse_cam(uint8_t *buf, uint32_t len, CAM_t **cam)
{
	return parse_something(buf, len, (void **)cam, &asn_DEF_CAM);
}

int parse_camv1(uint8_t *buf, uint32_t len, CAMv1_t **cam)
{
	return parse_something(buf, len, (void **)cam, &asn_DEF_CAMv1);
}

int parse_denm(uint8_t *buf, uint32_t len, DENM_t **denm)
{
	return parse_something(buf, len, (void **)denm, &asn_DEF_DENM);
}

int parse_mapem(uint8_t *buf, uint32_t len, MAPEM_t **mapem)
{
	return parse_something(buf, len, (void **)mapem, &asn_DEF_MAPEM);
}

int parse_spatem(uint8_t *buf, uint32_t len, SPATEM_t **spatem)
{
	return parse_something(buf, len, (void **)spatem, &asn_DEF_SPATEM);
}
