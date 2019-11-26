#ifndef __PARSER_H
#define __PARSER_H

#include <cstdint>
#include <string>

//#include "forwards.h"
#include "CAM.h"
#include "DENM.h"
#include "MAPEM.h"
#include "SPATEM.h"

#define ETHERTYPE_GEONET 0x8947

typedef enum __btpb_port
{
	BTP_B_PORT_CAM = 2001,
	BTP_B_PORT_DENM = 2002,
	BTP_B_PORT_MAPEM = 2003,
	BTP_B_PORT_SPATEM = 2004,
	BTP_B_PORT_SAEM = 2005,
	BTP_B_PORT_IVIM = 2006,
	BTP_B_PORT_SREM = 2007,
	BTP_B_PORT_SSEM = 2008,
} btpb_port_t;

typedef struct __ethernet
{
	uint8_t destination_mac[6];
	uint8_t source_mac[6];
	uint16_t type;
	uint8_t data[];
} __attribute__((packed)) ethernet_t;

typedef enum
{
	GEONET_BASIC_HEADER_NEXT_ANY = 0,
	GEONET_BASIC_HEADER_NEXT_COMMON = 1,
	GEONET_BASIC_HEADER_NEXT_SECURED = 2,
} geonet_basic_header_next_t;

typedef enum
{
	GEONET_BASIC_HEADER_LIFETIME_BASE_50MS = 0,
	GEONET_BASIC_HEADER_LIFETIME_BASE_1S = 1,
	GEONET_BASIC_HEADER_LIFETIME_BASE_10S = 2,
	GEONET_BASIC_HEADER_LIFETIME_BASE_100S = 3,
} geonet_basic_header_lifetime_base_t;

typedef enum
{
	GEONET_COMMON_HEADER_NEXT_ANY = 0,
	GEONET_COMMON_HEADER_NEXT_BTP_A = 1,
	GEONET_COMMON_HEADER_NEXT_BTP_B = 2,
	GEONET_COMMON_HEADER_NEXT_IPV6 = 3,
} geonet_common_header_next_t;

typedef enum __geonet_type
{
	GEONET_TYPE_ANY = 0x00,
	GEONET_TYPE_BEACON = 0x10,
	GEONET_TYPE_GUC = 0x20,
	GEONET_TYPE_GAC_CIRCLE = 0x30,
	GEONET_TYPE_GAC_RECT = 0x31,
	GEONET_TYPE_GAC_ELLIPSE = 0x32,
	GEONET_TYPE_GBC_CIRCLE = 0x40,
	GEONET_TYPE_GBC_RECT = 0x41,
	GEONET_TYPE_GBC_ELLIPSE = 0x42,
	GEONET_TYPE_TSB_SHB = 0x50,
	GEONET_TYPE_TSB_MHB = 0x51,
	GEONET_TYPE_LS_REQUEST = 0x60,
	GEONET_TYPE_LS_REPLY = 0x61,
} geonet_type_t;

typedef struct __geonetworking
{
	struct {
		uint8_t next_header:4;
		uint8_t version:4;
		uint8_t reserved;
		union {
			uint8_t raw;
			struct {
				uint8_t base:2;
				uint8_t mult:6;
			} fields;
		} life_time;
		uint8_t remaining_hop_limit;
	} basic_header;
	struct {
		uint8_t reserved:4;
		uint8_t next_header:4;
		union {
			uint8_t raw;
			struct {
				uint8_t sub_type:4;
				uint8_t type:4;
			} fields;
		} type;
		union {
			uint8_t raw;
			struct {
				uint8_t id:6;
				uint8_t offload:1;
				uint8_t scf:1;
			} fields;
		} traffic_class;
		union {
			uint8_t raw;
			struct {
				uint8_t todo;
			} fields;
		} flags;
		uint16_t payload_length;
		uint8_t max_hop_limit;
		uint8_t reserved2;
	} common_header;
	uint8_t data[];
} __attribute__((packed)) geonetworking_t;

typedef struct __geonet_long_position_vector
{
	uint64_t address;
	uint32_t timestamp;
	uint32_t latitude;
	uint32_t longitude;
	struct {
		uint8_t accuracy:1;
		uint16_t speed:15;
	} accuracy_speed;
	uint16_t heading;
} __attribute__((packed)) geonet_long_position_vector_t;

typedef struct __geonet_short_position_vector
{
	uint64_t address;
	uint32_t timestamp;
	uint32_t latitude;
	uint32_t longitude;
} __attribute__((packed)) geonet_short_position_vector_t;

typedef struct
{
	geonet_long_position_vector_t source_position;
	uint8_t data[];
} __attribute__((packed)) geonet_beacon_t;

typedef struct
{
	uint16_t sequence_number;
	uint16_t reserved;
	geonet_long_position_vector_t source_position;
	geonet_short_position_vector_t destination;
	uint8_t data[];
} __attribute__((packed)) geonet_guc_t;

typedef struct
{
	uint16_t sequence_number;
	uint16_t reserved;
	geonet_long_position_vector_t source_position;
	uint32_t latitude;
	uint32_t longitude;
	uint16_t distance_a;
	uint16_t distance_b;
	uint16_t angle;
	uint16_t reserved2;
	uint8_t data[];
} __attribute__((packed)) geonet_gac_t;

typedef struct
{
	uint16_t sequence_number;
	uint16_t reserved;
	geonet_long_position_vector_t source_position;
	uint8_t data[];
} __attribute__((packed)) geonet_tsb_mhb_t;

typedef struct
{
	geonet_long_position_vector_t source_position;
	uint32_t media_dependent_data;
	uint8_t data[];
} __attribute__((packed)) geonet_tsb_shb_t;



typedef struct __btp_b
{
	uint16_t port;
	uint16_t port_info;
	uint8_t data[];
} btp_b_t;

int btp_offset(uint8_t *buf, uint32_t len);

int dump_packet(uint8_t *buf, uint32_t len);

bool is_cam(uint8_t *buf, uint32_t len, uint32_t *cam_start);
bool is_denm(uint8_t *buf, uint32_t len, uint32_t *denm_start);
bool is_mapem(uint8_t *buf, uint32_t len, uint32_t *mapem_start);
bool is_spatem(uint8_t *buf, uint32_t len, uint32_t *spatem_start);

geonetworking_t *get_geonet_ptr(uint8_t);

int parse_header(uint8_t *buf, uint32_t len, ItsPduHeader_t **header);
int parse_cam(uint8_t *buf, uint32_t len, CAM_t **cam);
int parse_denm(uint8_t *buf, uint32_t len, DENM_t **denm);
int parse_mapem(uint8_t *buf, uint32_t len, MAPEM_t **mapem);
int parse_spatem(uint8_t *buf, uint32_t len, SPATEM_t **spatem);

#endif
