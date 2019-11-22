#include "factory.h"

#include <iostream>
#include <chrono>

uint64_t timestamp_now()
{
    static const uint64_t EPOCH_OFFSET = 1072911600000;
    #if __cpp_lib_chrono >= 201907
    auto t = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::utc_clock::now().time_since_epoch()).count();
    #else
    #warning No c++20 chrono utc_clock available! Falling back to system_clock
    auto t = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    #endif

    return t - EPOCH_OFFSET;
}

PacketFactory::PacketFactory(const uint8_t mac[6])
{
    buf = (uint8_t *) calloc(1, MAX_BUF_LEN);
    header_len = sizeof(ethernet_t) + sizeof(geonetworking_t) + sizeof(geonet_shb_t) + sizeof(btp_b_t);
    buflen = header_len;
    payload_len = sizeof(btp_b_t);

    e = (ethernet_t *) buf;
    g = (geonetworking_t *) e->data;
    s = (geonet_shb_t *) g->data;
    b = (btp_b_t *) s->data;

    memset(e->destination_mac, 0xFF, sizeof(e->destination_mac));
    e->source_mac[0] = mac[0];
    e->source_mac[1] = mac[1];
    e->source_mac[2] = mac[2];
    e->source_mac[3] = mac[3];
    e->source_mac[4] = mac[4];
    e->source_mac[5] = mac[5];

    e->type = htons(ETHERTYPE_GEONET);

    g->basic_header.version = 1;
    g->basic_header.next_header = 1;
    g->basic_header.life_time = 26;
    g->basic_header.remaining_hop_limit = 1;

    g->common_header.next_header = 2;
    g->common_header.type.raw = GEONET_TYPE_SHB;
    g->common_header.payload_length = htons(payload_len);
    g->common_header.max_hop_limit = 1;
}

PacketFactory::~PacketFactory()
{
    free(buf);
}


void PacketFactory::set_location(double lat, double lon)
{
    s->source_position.latitude = htonl(lat * 10000000.0);
    s->source_position.longitude = htonl(lon * 10000000.0);
}

void PacketFactory::set_timestamp(uint64_t timestamp)
{
    s->source_position.timestamp = (uint32_t)(timestamp % UINT32_MAX);
}

uint8_t *PacketFactory::get_raw()
{
    return buf;
}

uint32_t PacketFactory::get_len()
{
    return buflen;
}

CAMFactory::CAMFactory(const uint8_t mac[6]) : PacketFactory(mac)
{
    b->port = htons(BTP_B_PORT_CAM);

    cam = (CAM_t *) calloc(1, sizeof(CAM_t));
    cam->header.protocolVersion =2;
    cam->header.messageID = ItsPduHeader__messageID_cam;
}

CAMFactory::~CAMFactory()
{
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

/**
 * @param lat Latitude in degree
 * @param lon Longitude in degree
 * @param altitude Altitude in m
 */
void CAMFactory::set_location(double lat, double lon, int32_t altitude)
{
    PacketFactory::set_location(lat, lon);
    auto &r = cam->cam.camParameters.basicContainer.referencePosition;
    r.latitude = lat * 10000000.0;
    r.longitude = lon * 10000000.0;
    r.altitude.altitudeValue = altitude * 100;
    r.altitude.altitudeConfidence = AltitudeConfidence_unavailable;
}

void CAMFactory::set_timestamp(uint64_t timestamp)
{
    PacketFactory::set_timestamp(timestamp);
    cam->cam.generationDeltaTime = (uint16_t)(timestamp % UINT16_MAX);
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
            c.curvature.curvatureValue = CurvatureValue_unavailable;
            c.curvature.curvatureConfidence = CurvatureConfidence_unavailable;
            c.curvatureCalculationMode = CurvatureCalculationMode_unavailable;
            c.heading.headingValue = HeadingValue_unavailable;
            c.heading.headingConfidence = HeadingConfidence_unavailable;
            c.longitudinalAcceleration.longitudinalAccelerationValue = LongitudinalAccelerationValue_unavailable;
            c.longitudinalAcceleration.longitudinalAccelerationConfidence = AccelerationConfidence_unavailable;
            c.speed.speedValue = SpeedValue_unavailable;
            c.speed.speedConfidence = SpeedConfidence_unavailable;
            c.vehicleLength.vehicleLengthValue = VehicleLengthValue_unavailable;
            c.vehicleLength.vehicleLengthConfidenceIndication = VehicleLengthConfidenceIndication_unavailable;
            c.vehicleWidth = VehicleWidth_unavailable;
            c.yawRate.yawRateValue = YawRateValue_unavailable;
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

DENMFactory::DENMFactory(const uint8_t mac[6]) : PacketFactory(mac)
{
    b->port = htons(BTP_B_PORT_DENM);
    denm = (DENM_t *) calloc(1, sizeof(DENM_t));
    denm->header.protocolVersion = 2;
    denm->header.messageID = ItsPduHeader__messageID_denm;

    denm->denm.management.eventPosition.altitude.altitudeConfidence = AltitudeConfidence_unavailable;
}

DENMFactory::~DENMFactory()
{
    free(denm);
}

void DENMFactory::build_packet()
{
    asn_enc_rval_t r = uper_encode_to_buffer(&asn_DEF_DENM, nullptr, denm, buf + header_len, MAX_BUF_LEN-header_len);
    if (r.encoded == -1)
    {
        std::cout << "error building packet: " << (r.failed_type ? r.failed_type->name : "") << std::endl;
        return;
    }
    payload_len = sizeof(btp_b_t) + ((r.encoded + 7) / 8);
    buflen = header_len + payload_len - sizeof(btp_b_t);
    g->common_header.payload_length = htons(payload_len);
}

void DENMFactory::set_station_id(StationID_t id)
{
    denm->header.stationID = id;
}

void DENMFactory::set_station_type(StationType_t type)
{
    denm->denm.management.stationType = type;
}

void DENMFactory::set_detection_timestamp(uint64_t timestamp)
{
    asn_ulong2INTEGER(&denm->denm.management.detectionTime, timestamp);
}

void DENMFactory::set_reference_timestamp(uint64_t timestamp)
{
    asn_ulong2INTEGER(&denm->denm.management.referenceTime, timestamp);
}

/**
 *
 * @param lat Latitude in degree
 * @param lon Longitude in degree
 * @param altitude Altitude in m
 */
void DENMFactory::set_event_location(double lat, double lon, int32_t altitude)
{
    auto &e = denm->denm.management.eventPosition;
    e.latitude = lat * 10000000.0;
    e.longitude = lon * 10000000.0;
    e.altitude.altitudeValue = altitude * 100;
}

void DENMFactory::set_action_id(StationID_t orig, SequenceNumber_t seq)
{
    denm->denm.management.actionID.originatingStationID = orig;
    denm->denm.management.actionID.sequenceNumber = seq;
}

void DENMFactory::add_situation(InformationQuality_t quality, CauseCodeType_t causeType, SubCauseCodeType_t subCauseType)
{
    if (denm->denm.situation != nullptr)
    {
        free(denm->denm.situation);
    }

    denm->denm.situation = static_cast<SituationContainer *>(calloc(1, sizeof(SituationContainer_t)));

    denm->denm.situation->informationQuality = quality;
    denm->denm.situation->eventType.causeCode = causeType;
    denm->denm.situation->eventType.subCauseCode = subCauseType;
}
