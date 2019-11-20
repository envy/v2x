#include "factory.h"

#include <iostream>
#include <ctime>

#include "StationType.h"
#include "HighFrequencyContainer.h"
#include "HeadingConfidence.h"
#include "SpeedConfidence.h"
#include "VehicleLengthValue.h"

CAMFactory::CAMFactory()
{
	buf = (uint8_t *)calloc(1, MAX_BUF_LEN);
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
	cam->header.messageID = 2;
}

CAMFactory::~CAMFactory()
{
    free(buf);
    free(cam);
}

void CAMFactory::build_packet()
{
	asn_enc_rval_t r = uper_encode_to_buffer(&asn_DEF_CAM, nullptr, cam, buf + header_len, MAX_BUF_LEN-header_len);
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


/**
 * @param lat Latitude in degree
 * @param lon Longitude in degree
 * @param altitude Altitude in m
 */
void CAMFactory::set_location(double lat, double lon, int32_t altitude)
{
    s->source_position.latitude = lat * 10000000.0;
    s->source_position.longitude = lon * 10000000.0;

    cam->cam.camParameters.basicContainer.referencePosition.latitude = lat * 10000000.0;
    cam->cam.camParameters.basicContainer.referencePosition.longitude = lon * 10000000.0;
    cam->cam.camParameters.basicContainer.referencePosition.altitude.altitudeValue = altitude * 100;
    //cam->cam.camParameters.basicContainer.referencePosition.altitude.altitudeConfidence = ;
}

void CAMFactory::set_timestamp(uint32_t timestamp, uint16_t delta)
{
    s->source_position.timestamp = timestamp;
    cam->cam.generationDeltaTime = delta;
}

void CAMFactory::set_station_type(StationType_t type)
{
    cam->cam.camParameters.basicContainer.stationType = type;

    switch (type)
    {
        case StationType_unknown:
            break;
        case StationType_roadSideUnit:
            cam->cam.camParameters.highFrequencyContainer.present = HighFrequencyContainer_PR_basicVehicleContainerHighFrequency;
            cam->cam.camParameters.highFrequencyContainer.choice.rsuContainerHighFrequency.protectedCommunicationZonesRSU = nullptr;
            break;
        case StationType_pedestrian:
        case StationType_bus:
        case StationType_cyclist:
        case StationType_heavyTruck:
        case StationType_lightTruck:
        case StationType_moped:
        case StationType_motorcycle:
        case StationType_passengerCar:
        case StationType_specialVehicles:
        case StationType_tram: {
            auto &c = cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency;
            cam->cam.camParameters.highFrequencyContainer.present = HighFrequencyContainer_PR_basicVehicleContainerHighFrequency;
            c.curvature.curvatureConfidence = CurvatureConfidence_unavailable;
            c.curvatureCalculationMode = CurvatureCalculationMode_unavailable;
            c.heading.headingConfidence = HeadingConfidence_unavailable;
            c.longitudinalAcceleration.longitudinalAccelerationConfidence = AccelerationConfidence_unavailable;
            c.speed.speedConfidence = SpeedConfidence_unavailable;
            c.vehicleLength.vehicleLengthValue = VehicleLengthValue_unavailable;
            c.vehicleLength.vehicleLengthConfidenceIndication = VehicleLengthConfidenceIndication_unavailable;
            c.vehicleWidth = VehicleWidth_unavailable;
            c.yawRate.yawRateConfidence = YawRateConfidence_unavailable;
            break;
        }
        default:
            throw "Tried to set unhandled station type";
    }
}

void CAMFactory::set_station_id(StationID_t id)
{
    cam->header.stationID = id;
}
