#include "parser.h"
#include "proxy.h"
#include "factory.h"
#include "CAM.h"

#include <unistd.h>
#include <iostream>
#include <thread>

void dump_geonet(uint8_t *buf, uint32_t len)
{
	auto *e = (ethernet_t *)buf;
	auto *g = (geonetworking_t *)e->data;

	std::cout << "GeoNet " << format_geonet_type((geonet_type_t) g->common_header.type.raw) << std::endl;

	switch (g->common_header.type.raw)
	{
		case GEONET_TYPE_SHB:
		case GEONET_TYPE_BEACON:
		{
			auto *p = (geonet_long_position_vector_t *)g->data;
			double lat = p->latitude / 10000000.0;
			double lon = p->longitude / 10000000.0;
			std::cout << "Location: ";
			std::cout << lat << ", " << lon << std::endl;
			std::cout << "Timestamp: " << p->timestamp << std::endl;
		}
	}
}

void dump_cam(CAM_t *cam)
{
	std::cout << "CAM v" << cam->header.protocolVersion << " from ";
	std::cout << cam->header.stationID;
	std::cout << " (" << format_cam_station_type(cam->cam.camParameters.basicContainer.stationType);
	std::cout << ") ∆ ";
	std::cout << cam->cam.generationDeltaTime;
	std::cout << std::endl;

	std::cout << "Location: ";
	double lat = cam->cam.camParameters.basicContainer.referencePosition.latitude / 10000000.0;
	double lon = cam->cam.camParameters.basicContainer.referencePosition.longitude / 10000000.0;
	std::cout << lat << ", " << lon;
	std::cout << std::endl;

	if (cam->cam.camParameters.highFrequencyContainer.present == HighFrequencyContainer_PR_basicVehicleContainerHighFrequency)
    {
	    auto &b = cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency;
	    std::cout << "Vehicle Length: " << format_vehicle_length(b.vehicleLength.vehicleLengthValue) << std::endl;
	    std::cout << "Vehicle Width: " << format_vehicle_width(b.vehicleWidth) << std::endl;
	    std::cout << "Speed: " << format_speed_value(b.speed.speedValue) << std::endl;
	    std::cout << "Heading: " << format_heading_value(b.heading.headingValue) << std::endl;
    }
}

void dump_denm(DENM_t *denm)
{
    std::cout << "DENM v" << denm->header.protocolVersion << " from ";
    std::cout << denm->header.stationID;
    std::cout << std::endl;
}
void asserts()
{
	assert(sizeof(ethernet_t) == 14);
	assert(sizeof(geonetworking_t) == 4+8);
	assert(sizeof(geonet_long_position_vector_t) == 24);
	assert(sizeof(geonet_beacon_t) == 24);
	assert(sizeof(geonet_shb_t) == 28);
	assert(sizeof(btp_b_t) == 4);
}

void send_cam(uint8_t mac[6], StationID_t id, Proxy *p)
{
    while (1)
    {
        CAMFactory r(mac);
        r.set_timestamp(timestamp_now());
        r.set_location(52.2732617, 10.5252691, 70);
        r.set_station_id(id);
        r.set_station_type(StationType_pedestrian);
        r.build_packet();
        p->send_packet(r.get_raw(), r.get_len());

        usleep(1000*1000);
    }
}

void send_denm(uint8_t mac[6], StationID_t id, Proxy *p)
{
    while (1)
    {
        uint32_t t = timestamp_now();
        DENMFactory d(mac);
        d.set_timestamp(t);
        d.set_detection_timestamp(t - 1000);
        d.set_reference_timestamp(t);
        d.set_location(52.2732617, 10.5252691);
        d.set_station_id(id);
        d.set_station_type(StationType_pedestrian);
        d.set_event_location(52.2732617, 10.5252691, 73);
        d.set_action_id(id, 1);
        d.add_situation(InformationQuality_lowest, CauseCodeType_hazardousLocation_SurfaceCondition, 0);
        d.build_packet();
        p->send_packet(d.get_raw(), d.get_len());

        usleep(1000*1000);
    }
}

#define SERVER "10.1.4.72"
#define PORT 17565
int main(int argc, char *argv[]) {
    Proxy p;
    int port = PORT;

    asserts();

    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <addr> [port]" << std::endl;
        return -1;
    }
    if (argc >= 3) {
        port = (int) strtoul(argv[2], nullptr, 10);
    }

    if (p.connect(argv[1], port)) {
        return -1;
    }

    StationID_t id = 1337;

    uint8_t mac[6];
    mac[0] = 0x24;
    mac[1] = 0xA4;
    mac[2] = 0x3C;
    mac[3] = 0x02;
    mac[4] = 0xB6;
    mac[5] = 0x00;

    std::thread camthread(send_cam, mac, id, &p);
    std::thread denmthread(send_denm, mac, id, &p);

	uint8_t buf[1024];
	uint32_t buflen = sizeof(buf);
	while((buflen = p.get_packet((uint8_t *)&buf, buflen)) >= 0)
	{
		std::cout << "=======" << std::endl;

		dump_geonet(buf, buflen);

		uint32_t start;

		if (is_cam(buf, buflen, &start))
		{
			//std::cout << "cam @ " << cam_start << std::endl;
			CAM_t *cam = nullptr;
			int ret = parse_cam(buf+start, buflen-start, &cam);
			if (ret == 0)
			{
				dump_cam(cam);
				//xer_fprint(stdout, &asn_DEF_CAM, cam);
				goto next_iter;
			}
		}

		if (is_denm(buf, buflen, &start))
        {
		    DENM_t *denm = nullptr;
            int ret = parse_denm(buf+start, buflen-start, &denm);
            if (ret == 0)
            {
                dump_denm(denm);
                //xer_fprint(stdout, &asn_DEF_DENM, denm);
                goto next_iter;
            }
        }

		std::cout << "unknown packet" << std::endl;
next_iter:
		buflen = sizeof(buf);
	}

	return 0;
}
