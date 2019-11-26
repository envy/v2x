#ifndef __FACTORY_H_
#define __FACTORY_H_

#include "parser.h"

static const uint32_t MAX_BUF_LEN = 1024;

uint64_t timestamp_now();

class PacketFactory
{
protected:
	uint8_t *buf;
	uint32_t header_len;
	uint32_t buflen;
	uint16_t payload_len;

	ethernet_t *e;
	geonetworking_t *g;
	geonet_tsb_shb_t *s;
	btp_b_t *b;

public:
	explicit PacketFactory(const uint8_t mac[6]);
	virtual ~PacketFactory();
	virtual void build_packet() = 0;
	virtual void set_location(double lat, double lon);
	virtual void set_timestamp(uint64_t timestamp);
	uint8_t *get_raw();
	uint32_t get_len();
};

class CAMFactory : public PacketFactory
{
private:
	CAM_t *cam;

public:
	explicit CAMFactory(const uint8_t mac[6]);
	~CAMFactory() override;

	void build_packet() override;
	void set_location(double lat, double lon, int32_t altitude);
	void set_timestamp(uint64_t timestamp) override;
	void set_station_type(StationType_t type);
	void set_station_id(StationID_t id);
};

class DENMFactory : public PacketFactory
{
private:
	DENM_t *denm;

public:
	explicit DENMFactory(const uint8_t mac[6]);
	~DENMFactory() override;

	void build_packet() override;
	void set_station_id(StationID_t id);
	void set_station_type(StationType_t type);
	void set_detection_timestamp(uint64_t timestamp);
	void set_reference_timestamp(uint64_t timestamp);
	void set_event_location(double lat, double lon, int32_t altitude);
	void set_action_id(StationID_t orig, SequenceNumber_t seq);
	void add_situation(InformationQuality_t quality, CauseCodeType_t causeType, SubCauseCodeType_t subCauseType);
};

#endif
