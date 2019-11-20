#include "factory.h"

#include <iostream>
#include <ctime>

#include "StationType.h"
#include "HighFrequencyContainer.h"
#include "HeadingConfidence.h"
#include "SpeedConfidence.h"
#include "VehicleLengthValue.h"

CAMFactory::CAMFactory(StationID_t station_id)
{
	buf = (uint8_t *)calloc(1, 1024);
	header_len = sizeof(ethernet_t) + sizeof(geonetworking_t) + sizeof(geonet_shb_t) + sizeof(btp_b_t);
	buflen = header_len;
	payload_len = sizeof(btp_b_t);

	e = (ethernet_t *)buf;
	g = (geonetworking_t *)e->data;
	s = (geonet_shb_t *)g->data;
	b = (btp_b_t *)s->data;

	memset(e->destination_mac, 0xFF, sizeof(e->destination_mac));
	e->source_mac[0] = 0x24;
	e->source_mac[1] = 0xA4;
	e->source_mac[2] = 0x3C;
	e->source_mac[3] = 0x02;
	e->source_mac[4] = 0xB6;
	e->source_mac[5] = 0x00;

	e->type = htons(ETHERTYPE_GEONET);

	g->basic_header.version = 1;
	g->basic_header.next_header = 1;
	g->basic_header.life_time = 26;
	g->basic_header.remaining_hop_limit = 1;

	g->common_header.next_header = 2;
	g->common_header.type.raw = GEONET_TYPE_SHB;
	g->common_header.payload_length = htons(payload_len);
	g->common_header.max_hop_limit = 1;

	b->port = htons(BTP_B_PORT_CAM);

	cam = (CAM_t *)calloc(1, sizeof(CAM_t));
	cam->header.protocolVersion = 2;
	cam->header.stationID = station_id;
	cam->header.messageID = 2;

	cam->cam.camParameters.basicContainer.stationType = StationType_pedestrian;
	cam->cam.camParameters.highFrequencyContainer.present = HighFrequencyContainer_PR_basicVehicleContainerHighFrequency;
	cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.heading.headingConfidence = HeadingConfidence_unavailable;
	cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.speed.speedConfidence = SpeedConfidence_unavailable;
	cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.vehicleLength.vehicleLengthValue = VehicleLengthValue_unavailable;
	cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.vehicleWidth = VehicleWidth_unavailable;
}

void CAMFactory::set_location()
{
	s->source_position.timestamp = 771994298;
	s->source_position.latitude = 522732617;
	s->source_position.longitude = 105252691;

	cam->cam.generationDeltaTime = 1000;
	cam->cam.camParameters.basicContainer.referencePosition.latitude = 522732617;
	cam->cam.camParameters.basicContainer.referencePosition.longitude = 105252691;
	cam->cam.camParameters.basicContainer.referencePosition.altitude.altitudeValue = 7000;
	//cam->cam.camParameters.basicContainer.referencePosition.altitude.altitudeConfidence = ;
}

void CAMFactory::build_packet()
{
	asn_enc_rval_t r = uper_encode_to_buffer(&asn_DEF_CAM, NULL, cam, buf + header_len, 1024-header_len);
	if (r.encoded == -1)
	{
		std::cout << "error building packet: " << (r.failed_type ? r.failed_type->name : "") << std::endl;
		return;
	}
	payload_len = sizeof(btp_b_t) + ((r.encoded + 7) / 8);
	buflen = header_len + payload_len - sizeof(btp_b_t);
	g->common_header.payload_length = htons(payload_len);
}

uint8_t *CAMFactory::get_raw()
{
	return buf;
}

uint32_t CAMFactory::get_len()
{
	return buflen;
}
