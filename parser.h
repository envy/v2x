#ifndef __PARSER_H
#define __PARSER_H

#include <cstdint>
#include <string>

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

typedef enum __geonet_type
{
	GEONET_TYPE_BEACON = 0x10,
	GEONET_TYPE_SHB = 0x50,
} geonet_type_t;

typedef struct __ethernet
{
	uint8_t destination_mac[6];
	uint8_t source_mac[6];
	uint16_t type;
	uint8_t data[];
} __attribute__((packed)) ethernet_t;

typedef struct __geonetworking
{
	struct {
		uint8_t next_header:4;
		uint8_t version:4;
		uint8_t reserved;
		uint8_t life_time;
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
		uint8_t traffic_class;
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
	uint16_t accuracy_speed;
	uint16_t heading;
} __attribute__((packed)) geonet_long_position_vector_t;

typedef struct __geonet_shb
{
	geonet_long_position_vector_t source_position;
	uint32_t media_dependent_data;
	uint8_t data[];
} __attribute__((packed)) geonet_shb_t;

typedef struct __geonet_beacon
{
	geonet_long_position_vector_t source_position;
	uint8_t data[];
} __attribute__((packed)) geonet_beacon_t;

typedef struct __btp_b
{
	uint16_t port;
	uint16_t port_info;
	uint8_t data[];
} btp_b_t;

std::string format_station_type(StationType_t type);
std::string format_geonet_type(geonet_type_t type);
std::string format_vehicle_length(VehicleLengthValue_t val);
std::string format_vehicle_width(VehicleWidth_t val);
std::string format_speed_value(SpeedValue_t val);
std::string format_heading_value(HeadingValue_t val);
std::string format_event_state(MovementPhaseState_t val);
std::string format_cause_code(CauseCode_t &val);
std::string format_protected_zone_type(ProtectedZoneType_t val);
std::string format_lane_direction(LaneDirection_t &val);
std::string format_lane_type(LaneTypeAttributes_t &val);

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
