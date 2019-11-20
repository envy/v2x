#include "parser.h"
#include "proxy.h"
#include "factory.h"
#include "CAM.h"


#include <iostream>

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
	std::cout << ") @ ";
	std::cout << cam->cam.generationDeltaTime;
	std::cout << std::endl;

	std::cout << "Location: ";
	double lat = cam->cam.camParameters.basicContainer.referencePosition.latitude / 10000000.0;
	double lon = cam->cam.camParameters.basicContainer.referencePosition.longitude / 10000000.0;
	std::cout << lat << ", " << lon;
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

#define SERVER "10.1.4.72"
#define PORT 17565
int main(int argc, char *argv[])
{
    Proxy p;
    int port = PORT;

    asserts();

	if (argc < 2)
    {
	    std::cout << "Usage: " << argv[0] << " <addr> [port]" << std::endl;
	    return -1;
    }
	if (argc >= 3)
    {
        port = (int)strtoul(argv[2], nullptr, 10);
    }

	if (p.connect(argv[1], port))
    {
	    return -1;
    }

	CAMFactory r;

	r.set_timestamp(771994298, 1000);
	r.set_location(52.2732617, 10.5252691, 70);
	r.set_station_id(1337);
	r.set_station_type(StationType_pedestrian);

	r.build_packet();

	p.send_packet(r.get_raw(), r.get_len());

	uint8_t buf[1024];
	uint32_t buflen = sizeof(buf);
	while((buflen = p.get_packet((uint8_t *)&buf, buflen)) >= 0)
	{
		std::cout << "=======" << std::endl;

		dump_geonet(buf, buflen);

		uint32_t cam_start;

		if (is_cam(buf, buflen, &cam_start))
		{
			//std::cout << "cam @ " << cam_start << std::endl;
			CAM_t *cam = nullptr;
			int ret = parse_cam(buf+cam_start, buflen-cam_start, &cam);
			if (ret == 0)
			{
				dump_cam(cam);
				xer_fprint(stdout, &asn_DEF_CAM, cam);
				goto next_iter;
			}
		}

		std::cout << "unknown packet" << std::endl;
next_iter:
		buflen = sizeof(buf);
	}

	return 0;
}
