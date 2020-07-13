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


	tlen -= sizeof(geonetworking_t);
	geonetworking_common_header_t *c = nullptr;
	if (g->basic_header.next_header == GEONET_BASIC_HEADER_NEXT_SECURED)
	{
		auto *s = (geonetworking_secured_packet_t *)g->data;

		tlen -=sizeof(geonetworking_secured_packet_t);
		c = (geonetworking_common_header_t *)s->data;
		std::cout << "Next Header: " << (unsigned int)c->next_header << std::endl;
		std::cout << "Type: 0x" << std::hex << (unsigned int)c->type.raw << std::dec << std::endl;
		std::cout << "Traffic Class: " << (unsigned int)c->traffic_class.raw << std::endl;
		std::cout << "Payload length: " << (unsigned int)ntohs(c->payload_length) << std::endl;
		std::cout << "Max Hop Limit: " << (unsigned int)c->max_hop_limit << std::endl;

		tlen -= sizeof(geonetworking_common_header_t);
	}
	else if (g->basic_header.next_header == GEONET_BASIC_HEADER_NEXT_COMMON)
	{
		c = (geonetworking_common_header_t *)g->data;
		std::cout << "Next Header: " << (unsigned int)c->next_header << std::endl;
		std::cout << "Type: 0x" << std::hex << (unsigned int)c->type.raw << std::dec << std::endl;
		std::cout << "Traffic Class: " << (unsigned int)c->traffic_class.raw << std::endl;
		std::cout << "Payload length: " << (unsigned int)ntohs(c->payload_length) << std::endl;
		std::cout << "Max Hop Limit: " << (unsigned int)c->max_hop_limit << std::endl;

		tlen -= sizeof(geonetworking_common_header_t);
	}

	// TODO: check next header == btp-b

	std::cout << tlen << " remaining" << std::endl;

	if (tlen < sizeof(btp_b_t))
	{
		std::cout << "Not enough data for btp-b header" << std::endl;
		return -6;
	}

	auto *b = (btp_b_t *)(c->data + sizeof(geonet_tsb_shb_t));

	std::cout << "Port: " << (unsigned int)ntohs(b->port) << std::endl;
	std::cout << "Port Info: " << std::hex << (unsigned int)ntohs(b->port_info) << std::dec << std::endl;

	return 0;
}

int ch_offset(uint8_t *buf, int avail)
{
	int offset = 0;
	// we start with a Ieee1609Dot2Data
	// first octet is protocol version
	std::cout << "protocol version: " << (int)buf[offset] << std::endl;
	offset++;

	// next is the Ieee1609Dot2Content choice
	std::cout << "Ieee1609Dot2Content choice: 0x" << std::hex << (int)buf[offset] << std::dec << std::endl;
	// it should be set to 0x81
	if (buf[offset] != 0x81)
	{
		std::cerr << "unexpected Ieee1609Dot2Content choice in outer data" << std::endl;
		exit(1);
	}
	offset++;

	// next is SignedData
	// first in that is hashId, an enum
	std::cout << "hashId: " << (int)buf[offset] << std::endl;
	offset++;

	// then we have the tbsData
	// first in that is payload
	// first in that is the presence bitmap of the sequence
	std::cout << "SignedDataPayload presence bitmap: 0x" << std::hex << (int)buf[offset] << std::dec << std::endl;
	// it should be 0x40
	if (buf[offset] != 0x40)
	{
		std::cerr << "unexpected SignedDataPayload presence bitmap in outer data" << std::endl;
		exit(1);
	}
	offset++;

	// then we again have a Ieee1609Dot2Data
	// first octet is protocol version
	std::cout << "protocol version: " << (int)buf[offset] << std::endl;
	offset++;

	// next is the Ieee1609Dot2Content choice
	std::cout << "Ieee1609Dot2Content choice: 0x" << std::hex << (int)buf[offset] << std::dec << std::endl;
	// it should be set to 0x80
	if (buf[offset] != 0x80)
	{
		std::cerr << "unexpected Ieee1609Dot2Content choice in inner data" << std::endl;
		exit(1);
	}
	offset++;

	// next is Opaque
	// this is a octet string with variable size, so a length prefix first
	if (buf[offset] & 0b10000000)
	{
		// there is an octet telling us how many octets to read
		auto to_read = static_cast<uint8_t>(buf[offset] & 0b01111111);
		std::cout << "need to read " << (int)to_read << " octets" << std::endl;
		offset += 1 + to_read;
	}
	else
	{
		std::cout << "need to read 1 octet" << std::endl;
		offset++;
	}

	return offset;
}

int btp_offset(uint8_t *buf, uint32_t len)
{
	uint32_t fix_offset = sizeof(ethernet_t) + sizeof(geonetworking_t);
	if (len < fix_offset)
	{
		return -1;
	}
	auto *e = (ethernet_t *)buf;
	auto *g = (geonetworking_t *)e->data;
	geonetworking_common_header_t *c = nullptr;
	if (g->basic_header.next_header == 1)
	{
		c = (geonetworking_common_header_t *)g->data;
		fix_offset += sizeof(geonetworking_common_header_t);
	}
	else
	{
		int sec = 0;
		if ((sec = ch_offset(g->data, len)) < 0)
		{
			return -1;
		}
		c = (geonetworking_common_header_t *)(g->data + sec);
		fix_offset += sec;
		fix_offset += sizeof(geonetworking_common_header_t);
	}
	switch (c->type.raw)
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
			std::cerr << "PARSER FIXME: unknown geonet type 0x" << std::hex << (unsigned int)c->type.raw << std::dec << std::endl;
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

int parse_denmv1(uint8_t *buf, uint32_t len, DENMv1_t **denm)
{
	return parse_something(buf, len, (void **)denm, &asn_DEF_DENMv1);
}

int parse_mapem(uint8_t *buf, uint32_t len, MAPEM_t **mapem)
{
	return parse_something(buf, len, (void **)mapem, &asn_DEF_MAPEM);
}

int parse_spatem(uint8_t *buf, uint32_t len, SPATEM_t **spatem)
{
	return parse_something(buf, len, (void **)spatem, &asn_DEF_SPATEM);
}
