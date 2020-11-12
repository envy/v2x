#include "parser.h"
#include "Formatter.h"

#include <iostream>
#include <arpa/inet.h>

#include "asn1-src/Ieee1609Dot2Data.h"
#include "asn1-src/Ieee1609Dot2Content.h"
#include "asn1-src/SignedData.h"
#include "asn1-src/SignedDataPayload.h"
#include "asn1-src/ToBeSignedData.h"

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

int geonet_size(uint8_t type)
{
	switch (type)
	{
	case GEONET_TYPE_BEACON:
		return sizeof(geonet_beacon_t);
	case GEONET_TYPE_GUC:
		return sizeof(geonet_guc_t);
	case GEONET_TYPE_GAC_CIRCLE:
	case GEONET_TYPE_GAC_ELLIPSE:
	case GEONET_TYPE_GAC_RECT:
	case GEONET_TYPE_GBC_CIRCLE:
	case GEONET_TYPE_GBC_ELLIPSE:
	case GEONET_TYPE_GBC_RECT:
		return sizeof(geonet_gac_t);
	case GEONET_TYPE_TSB_MHB:
		return sizeof(geonet_tsb_mhb_t);
	case GEONET_TYPE_TSB_SHB:
		return sizeof(geonet_tsb_shb_t);
	default:
		std::cerr << "PARSER FIXME: unknown geonet type 0x" << std::hex << (unsigned int)type << std::dec << std::endl;
		return 0;
	}
}

int btp_offset(uint8_t *buf, uint32_t len)
{
	uint32_t header_size = sizeof(ethernet_t) + sizeof(geonetworking_t);
	if (len < header_size)
	{
		return -1;
	}
	auto *e = (ethernet_t *)buf;
	auto *g = (geonetworking_t *)e->data;
	if (g->basic_header.next_header == GEONET_BASIC_HEADER_NEXT_ANY)
	{
		std::cerr << "PARSER FIXME: btp_offset for ANY header?!" << std::endl;
		return -2;
	}
	if (g->basic_header.next_header == GEONET_BASIC_HEADER_NEXT_COMMON)
	{
		auto *c = (geonetworking_common_header_t *)g->data;
		header_size += sizeof(geonetworking_common_header_t);
		header_size += geonet_size(c->type.raw);

		if (c->next_header == GEONET_COMMON_HEADER_NEXT_BTP_B || c->next_header == GEONET_COMMON_HEADER_NEXT_BTP_A)
		{
			return header_size;
		}
		if (c->type.raw != GEONET_TYPE_BEACON)
		{
			std::cerr << "PARSER: common next header is not BTP and type is not BEACON" << std::endl;
		}
		return -3;
	}
	else if (g->basic_header.next_header == GEONET_BASIC_HEADER_NEXT_SECURED)
	{
		asn_dec_rval_t ret;
		Ieee1609Dot2Data_t *data = nullptr;

		ASN_STRUCT_RESET(asn_DEF_Ieee1609Dot2Data, data);
		ret = oer_decode(nullptr, &asn_DEF_Ieee1609Dot2Data, (void **)&data, g->data, len);

		if (ret.code == RC_WMORE)
		{
			std::cerr << "PARSER FIXME: secured packet not comeplete!" << std::endl;
			return -4;
		}
		if (ret.code == RC_FAIL)
		{
			ASN_STRUCT_FREE(asn_DEF_Ieee1609Dot2Data, data);
			std::cerr << "PARSER FIXME: secured packet asn decode failure" << std::endl;
			return -5;
		}
		// ret.code == RC_OK here
		// hacky: we will find the unsecured data and then copy it directly after the basic header and point to that.
		// first we need to find the unsecured data
		while (true)
		{
			auto *content = data->content;
			if (content->present == Ieee1609Dot2Content_PR_unsecuredData)
			{
				// found it!
				auto unsecdata = content->choice.unsecuredData;
				memcpy(g->data, unsecdata.buf, unsecdata.size);
				header_size += sizeof(geonetworking_common_header_t);
				auto *c = (geonetworking_common_header_t *)g->data;
				header_size += geonet_size(c->type.raw);
				ASN_STRUCT_FREE(asn_DEF_Ieee1609Dot2Data, data);
				if (c->next_header == GEONET_COMMON_HEADER_NEXT_BTP_B || c->next_header == GEONET_COMMON_HEADER_NEXT_BTP_A)
				{
					return header_size;
				}
				if (c->type.raw != GEONET_TYPE_BEACON)
				{
					std::cerr << "PARSER: common next header is not BTP and type is not BEACON" << std::endl;
				}
				return -6;
			}
			if (content->present == Ieee1609Dot2Content_PR_signedData)
			{
				// signed data, need to go through a lot of datattypes
				auto *signeddata = content->choice.signedData;
				auto *tbsdata = signeddata->tbsData;
				auto *signeddatapayload = tbsdata->payload;
				data = signeddatapayload->data;
				// at the end, we again have a Ieee1609Dot2Data_t pointer
				continue;
			}
		}
	}

	std::cerr << "PARSER unknown next header " << (int)g->basic_header.next_header << std::endl;
	return -7;
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

bool is_beacon(uint8_t *buf, uint32_t len, uint32_t *beacon_start)
{
	auto header_len = sizeof(ethernet_t) + sizeof(geonetworking_t);
	if (header_len > len)
	{
		return false;
	}

	auto *e = (ethernet_t *)buf;
	auto *g = (geonetworking_t *)e->data;
	if (g->basic_header.next_header == GEONET_BASIC_HEADER_NEXT_COMMON)
	{
		auto *c = (geonetworking_common_header_t *)g->data;
		return c->type.raw == GEONET_TYPE_BEACON;
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
		std::cout << "PARSER: Could not decode: ";
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
