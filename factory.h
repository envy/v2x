#ifndef __FACTORY_H_
#define __FACTORY_H_

#include "parser.h"
#include "CAM.h"
#include "StationID.h"


class CAMFactory
{
private:
	uint8_t *buf;
	uint32_t header_len;
	uint32_t buflen;
	uint16_t payload_len;

	ethernet_t *e;
	geonetworking_t *g;
	geonet_shb_t *s;
	btp_b_t *b;

	CAM_t *cam;
public:
	CAMFactory(StationID_t station_id);

	void set_location();

	void build_packet();
	uint8_t *get_raw();
	uint32_t get_len();
};

#endif
