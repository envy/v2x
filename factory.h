#ifndef __FACTORY_H_
#define __FACTORY_H_

#include "parser.h"
#include "CAM.h"
#include "StationID.h"

static const uint32_t MAX_BUF_LEN = 1024;

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
	CAMFactory();
	virtual ~CAMFactory();

	void set_location(double lat, double lon, int32_t altitude);
	void set_timestamp(uint32_t timestamp, uint16_t delta);
	void set_station_type(StationType_t type);
	void set_station_id(StationID_t id);

	void build_packet();
	uint8_t *get_raw();
	uint32_t get_len();
};

#endif
