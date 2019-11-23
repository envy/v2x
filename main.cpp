#include "parser.h"
#include "proxy.h"
#include "factory.h"
#include "MessageSink.h"

#include <unistd.h>
#include <iostream>
#include <thread>

#include <SFML/Graphics.hpp>
#include <asn1-src/RoadSegmentList.h>
#include <asn1-src/RoadLaneSetList.h>
#include <asn1-src/ConnectsToList.h>

Proxy p;
MessageSink ms;

sf::Font font;

sf::RenderWindow *window = nullptr;

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
			double lat = ntohl(p->latitude) / 10000000.0;
			double lon = ntohl(p->longitude) / 10000000.0;
			std::cout << " Location: " << lat << ", " << lon << std::endl;
			std::cout << " Timestamp: " << p->timestamp << std::endl;
		}
	}
}

void dump_cam(CAM_t *cam)
{
	std::cout << "CAM v" << cam->header.protocolVersion << " from ";
	std::cout << cam->header.stationID;
	std::cout << " (" << format_station_type(cam->cam.camParameters.basicContainer.stationType);
	std::cout << ") ∆ ";
	std::cout << cam->cam.generationDeltaTime;
	std::cout << std::endl;

	std::cout << " Location: ";
	double lat = cam->cam.camParameters.basicContainer.referencePosition.latitude / 10000000.0;
	double lon = cam->cam.camParameters.basicContainer.referencePosition.longitude / 10000000.0;
	std::cout << lat << ", " << lon;
	std::cout << std::endl;

	if (cam->cam.camParameters.highFrequencyContainer.present == HighFrequencyContainer_PR_basicVehicleContainerHighFrequency)
	{
		auto &b = cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency;
		std::cout << " Vehicle Length: " << format_vehicle_length(b.vehicleLength.vehicleLengthValue) << std::endl;
		std::cout << " Vehicle Width: " << format_vehicle_width(b.vehicleWidth) << std::endl;
		std::cout << " Speed: " << format_speed_value(b.speed.speedValue) << std::endl;
		std::cout << " Heading: " << format_heading_value(b.heading.headingValue) << std::endl;
	}
	else if (cam->cam.camParameters.highFrequencyContainer.present == HighFrequencyContainer_PR_rsuContainerHighFrequency)
	{
		auto &r = cam->cam.camParameters.highFrequencyContainer.choice.rsuContainerHighFrequency;
		if (r.protectedCommunicationZonesRSU != nullptr)
		{
			for(uint32_t i = 0; i < r.protectedCommunicationZonesRSU->list.count; ++i)
			{
				auto item = r.protectedCommunicationZonesRSU->list.array[i];
				double _lat = item->protectedZoneLatitude / 10000000.0;
				double _lon = item->protectedZoneLongitude / 10000000.0;
				std::cout << " Protected Zone: " << format_protected_zone_type(item->protectedZoneType) << std::endl;
				std::cout << " Location: " << _lat << ", " << _lon << std::endl;
			}
		}
	}
}

void dump_denm(DENM_t *denm)
{
	std::cout << "DENM v" << denm->header.protocolVersion << " from ";
	std::cout << denm->header.stationID;
	std::cout << std::endl;

	if (denm->denm.situation != nullptr)
	{
		auto situation = denm->denm.situation;
		std::cout << " Cause: " << format_cause_code(situation->eventType) << std::endl;
		std::cout << " Info Quality: " << situation->informationQuality << std::endl;
	}
}

void dump_spatem(SPATEM_t *spatem)
{
	std::cout << "SPATEM v" << spatem->header.protocolVersion << " from ";
	std::cout << spatem->header.stationID;
	std::cout << std::endl;

	if (spatem->spat.timeStamp != nullptr)
	{
		std::cout << " Timestamp: " << *spatem->spat.timeStamp << std::endl;
	}

	for (uint32_t i = 0; i < spatem->spat.intersections.list.count; ++i)
	{
		auto intersection = spatem->spat.intersections.list.array[i];
		std::cout << " Intersection " << intersection->id.id << std::endl;
		for (uint32_t j = 0; j < intersection->states.list.count; ++j)
		{
			auto signal = intersection->states.list.array[j];
			std::cout << "  Signal Group " << signal->signalGroup << ": ";
			for (uint32_t k = 0; k < signal->state_time_speed.list.count; ++k)
			{
				if (k != 0)
				{
					std::cout << ", ";
				}
				auto state = signal->state_time_speed.list.array[k];
				std::cout << format_event_state(state->eventState);
			}
			std::cout << std::endl;
		}
	}
}

void dump_mapem(MAPEM_t *mapem)
{
	std::cout << "MAPEM v" << mapem->header.protocolVersion << " from ";
	std::cout << mapem->header.stationID;
	std::cout << std::endl;

	if (mapem->map.roadSegments != nullptr)
	{
		for (uint32_t rs = 0; rs < mapem->map.roadSegments->list.count; ++rs)
		{
			auto seg = mapem->map.roadSegments->list.array[rs];
			std::cout << " Road Segment: " << seg->id.id;
			if (seg->name != nullptr)
			{
				std::cout << " - " << std::string((char *)seg->name->buf, seg->name->size);
			}
			std::cout << std::endl;
			double lat = seg->refPoint.lat / 10000000.0;
			double lon = seg->refPoint.Long / 10000000.0;
			std::cout << "  Location: " << lat << ", " << lon << std::endl;
			for (uint32_t ls = 0; ls < seg->roadLaneSet.list.count; ++ls)
			{
				auto rl = seg->roadLaneSet.list.array[ls];
				std::cout << "  Lane: " << rl->laneID << std::endl;
				std::cout << "   Direction: " << format_lane_direction(rl->laneAttributes.directionalUse) << std::endl;
				std::cout << "   Type: " << format_lane_type(rl->laneAttributes.laneType) << std::endl;
				if (rl->connectsTo != nullptr)
				{
					std::cout << "   Connects to: ";
					for (uint32_t ct = 0; ct < rl->connectsTo->list.count; ++ct)
					{
						if (ct != 0)
						{
							std::cout << ", ";
						}
						auto con = rl->connectsTo->list.array[ct];
						std::cout << con->connectingLane.lane;
						if (con->signalGroup != nullptr)
						{
							std::cout << " (SG " << *con->signalGroup << ")";
						}
					}
					std::cout << std::endl;
				}
			}
		}
	}
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
	CAMFactory r(mac);
	r.set_timestamp(timestamp_now());
	r.set_location(52.2732617, 10.5252691, 70);
	r.set_station_id(id);
	r.set_station_type(StationType_pedestrian);
	r.build_packet();
	p->send_packet(r.get_raw(), r.get_len());
}

void send_cam_thread(uint8_t mac[6], StationID_t id, Proxy *p)
{
	while (1)
	{
		send_cam(mac, id, p);
		usleep(1000*1000);
	}
}

void send_denm(uint8_t mac[6], StationID_t id, Proxy *p)
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
}

void send_denm_thread(uint8_t mac[6], StationID_t id, Proxy *p)
{
	while (1)
	{
		send_denm(mac, id, p);
		usleep(1000*1000);
	}
}

void reader_thread()
{
	uint8_t buf[2048];
	uint32_t buflen = sizeof(buf);
	while((buflen = p.get_packet((uint8_t *)&buf, buflen)) >= 0)
	{
		array_t a = { buf, buflen };
		ms.add_msg(a);
		buflen = sizeof(buf);
	}
}

void draw_data()
{
	ms.draw_data();
}

void write(float x, float y, const sf::Color &color, const std::string &text)
{
	sf::Text t;
	t.setFont(font);
	t.setCharacterSize(24);
	t.setFillColor(color);
	//t.setOrigin(x, y);
	t.setPosition(x, y);
	t.setString(text);
	window->draw(t);
}

#define SERVER "10.1.4.72"
#define PORT 17565
int main(int argc, char *argv[]) {
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

	//std::thread camthread(send_cam_thread, mac, id, &p);
	//std::thread denmthread(send_denm_thread, mac, id, &p);

	uint8_t aspat[] = "\xff\xff\xff\xff\xff\xff\x00\x0d\x41\x12\x21\x4d\x89\x47\x01\x00" \
"\x1a\x01\x20\x50\x03\x00\x00\x8a\x01\x00\x3c\xe8\x00\x0d\x41\x12" \
"\x21\x4d\xc4\xaa\x78\x0e\x1f\x28\x8c\x7a\x06\x45\xe0\x84\x80\x19" \
"\x00\x1b\x00\x01\x00\x00\x07\xd4\x00\x00\x01\x04\x00\x12\x20\x4c" \
"\x00\x00\x00\x12\x01\x02\x00\x11\x00\x10\x43\x04\x09\xb8\x01\x02" \
"\x1b\xa0\x64\x60\xc3\x60\x93\xc0\x00\xc1\x0d\xd0\x32\x30\x61\xb0" \
"\x49\xe0\x00\x80\x8a\x08\x13\x70\x05\x04\x50\x40\xff\x80\x30\x21" \
"\x82\x04\xdc\x01\xc1\x14\x10\x26\xe0\x10\x08\x60\x81\xff\x00\x90" \
"\x45\x04\x09\xb8\x05\x02\x18\x20\x7f\xc0\x2c\x11\x41\x02\x6e\x01" \
"\x80\x86\xe8\x3a\x38\x3d\xf8\x3c\x1b\x00\xd0\x43\x74\x1d\x1c\x1e" \
"\xfc\x1e\x0d\x80\x70\x22\x82\x04\xdc\x03\xc1\x0c\x10\x3f\xe0\x20" \
"\x08\x60\x81\xff\x01\x10\x43\x04\x0f\xf8\x09\x02\x18\x20\x7f\xc0";
	uint8_t atopo[] = "\xff\xff\xff\xff\xff\xff\x00\x0d\x41\x12\x21\x4d\x89\x47\x01\x00" \
"\x1a\x01\x20\x50\x03\x00\x02\xe8\x01\x00\x3c\xe8\x00\x0d\x41\x12" \
"\x21\x4d\xc4\xaa\x77\x70\x1f\x28\x8c\x7a\x06\x45\xe0\x84\x80\x19" \
"\x00\x1b\x00\x01\x00\x00\x07\xd3\x00\x00\x01\x05\x00\x12\x20\x4c" \
"\x08\x00\x03\x02\x4c\xa7\x05\x9b\x68\x01\x88\x01\x20\x13\x53\x36" \
"\x0c\x39\xc6\x3e\xdf\x10\x03\x04\x0c\x35\x00\x00\x02\x94\x42\x68" \
"\xca\x22\xc0\x01\x00\x00\x00\x5e\x04\xdd\x20\x80\x02\x07\xe7\x8b" \
"\x05\x33\x06\x00\x9a\x33\x08\xb0\x00\x40\x00\x00\x07\x85\x94\x62" \
"\x82\x06\x56\x81\x4c\xa1\x60\x46\x8c\xe2\x2c\x00\x10\x00\x00\x05" \
"\xe1\x80\xf5\x28\x00\x20\x7e\x71\xc0\x53\x40\x70\x19\xa3\x40\x8b" \
"\x00\x04\x00\x00\x01\x78\xb4\x23\x5a\x00\x08\x20\x54\x24\x14\xce" \
"\x1a\x08\x68\xde\x22\xc0\x02\x00\x00\x00\x16\xc4\x39\x18\x1e\x7d" \
"\xf4\x14\xe0\x0e\x0a\x68\xe0\x22\xc0\x02\x00\x00\x00\x1d\xba\x45" \
"\x72\x08\x01\x5b\x05\x37\x84\x83\x12\x05\x8a\x00\x00\x00\x00\xbc" \
"\xc7\x38\x31\x00\x04\x9a\xfd\xc3\x0e\x0a\x26\x01\x07\x24\x0c\x14" \
"\x00\x00\x00\x01\x79\x93\x67\xea\x00\x09\x35\xe2\x85\x78\x14\x4a" \
"\x02\x10\x48\x1a\x28\x00\x00\x00\x02\xf3\x3e\xbb\xf4\x00\x12\x6c" \
"\x29\x0c\x00\x28\x90\x04\x24\x90\x38\x50\x00\x00\x00\x05\xe6\xe1" \
"\x51\x68\x00\x24\xd8\x7a\x17\x50\x50\xd0\x10\x51\x20\x78\xa0\x00" \
"\x00\x00\x0b\xcd\xda\x58\xd0\x00\x49\xb2\x94\x34\x80\xa1\x90\x20" \
"\xb1\x01\x01\x20\x00\x00\x00\x07\xa5\xe3\x9c\x8f\x8c\x30\x93\x10" \
"\x11\x12\x00\x00\x00\x00\x7a\x7e\x30\xf0\xf8\xf3\x0a\x81\x01\x21" \
"\x20\x00\x00\x00\x07\xa7\xf2\x6c\x8f\x99\x70\x9e\x24\x15\x24\x00" \
"\x00\x00\x05\x91\xb4\x99\x3c\x80\x02\x48\xa1\x8c\x90\x13\xe4\x03" \
"\x50\x28\x48\x0c\x30\x90\x58\x90\x00\x00\x00\x26\x44\x7a\x63\x8a" \
"\x00\x0b\x23\x79\x14\x9d\x00\x10\x6f\xb3\x31\x20\xe2\x42\x55\xf1" \
"\x50\x88\x18\x6a\x84\x00\xc3\x89\x05\xc9\x00\x00\x00\x02\x64\x21" \
"\x66\x1c\x20\x00\x92\x2b\x43\x35\x17\x09\x11\xa8\x20\x02\x0e\x1f" \
"\xe0\xb5\x15\x13\x02\x07\xa8\x94\x10\x40\x90\x60\x90\x00\x00\x00" \
"\x16\x3f\xe8\x60\x92\x00\x09\x23\x2c\x1f\xd0\x52\x62\x0a\xc0\xa2" \
"\x40\x41\x11\x01\x92\x20\x00\x00\x00\x08\xee\x89\x8e\xa4\x8a\x00" \
"\xcb\x81\x01\xa2\x20\x00\x00\x00\x08\xe3\x99\x8e\x24\x8a\x78\xca" \
"\xc3\x41\xc2\x26\x00\x10\x00\x00\x00\xe9\xc6\x00\x30\x6b\x50\x58" \
"\x28\x6c\x20\x48\xd0\x6c\x89\x80\x04\x00\x00\x00\x46\x54\x0f\x67" \
"\x00\x6c\xfe\x82\x87\x02\x84\xcd\x32\x48\x98\x00\x20\x00\x00\x0c" \
"\x8e\xe4\xe5\x54\x00\x10\x60\x70\x78\x2b\x28\x40\x50\xd3\x28\x89" \
"\x80\x02\x00\x00\x00\xc7\xc9\x4d\xf1\x40\x01\x02\x92\xf4\x82\xb2" \
"\x43\xc5\x4d\x32\xc8\x98\x00\x20\x00\x00\x0c\x78\x68\xde\x14\x00" \
"\x10\x56\x70\xd8\x2b\x30\x48\x58\xd3\x30\x89\x80\x02\x00\x00\x00" \
"\xc6\xe3\x8d\x91\x40\x01\x02\x7b\x00\x02\xb2\xc4\x45\xc9\x07\xcd" \
"\x00\x00\x00\x00\x59\x24\xc4\xb4\x80\x02\x54\xd7\x97\xd1\xd0\x50" \
"\xd0\x28\xc1\x21\x01\xa0\x00\x00\x00\x0b\x23\xf0\xe2\xd0\x00\x4a" \
"\x9b\x04\xf9\x90\x0a\x19\x05\x19\x24\x21\x34\x00\x00\x00\x01\x64" \
"\x84\x26\x7a\x00\x09\x53\x44\x9f\x2f\x41\x42\x40\xc3\x44\x84\x46" \
"\x80\x00\x00\x00\xac\x8c\x25\xfa\x40\x01\x6a\xf2\x93\xee\x0a\x00" \
"\x21\x0e\xbe\x7b\x84\x14\x22\x0c\x36\x48\x46\x68\x00\x00\x00\x0a" \
"\xc8\x86\x72\x34\x00\x12\x0e\xe8\xf2\xe0\xcd\xf8\xe3\xc0\x50\x80" \
"\x30\xe0\x81\x21\x90\x00\x00\x00\x03\x1f\x9a\x8e\x86\x56\xe7\x82" \
"\x08\x12\x99\x00\x00\x00\x00\x31\xfb\x2c\xbc\x65\x2a\x7b\x30\x81" \
"\x31\x90\x00\x00\x00\x03\x1e\xfb\x25\x46\x51\xc7\x8d\x80";
//*/

	///*
	uint32_t s = 0;
	//dump_packet(atopo, sizeof(atopo));
	dump_geonet(atopo, sizeof(atopo));
	if (is_mapem(atopo, sizeof(atopo), &s))
	{
		ItsPduHeader_t *header = nullptr;
		int ret = parse_header(atopo+s, sizeof(atopo)-s, &header);
		if (ret == 0)
		{
			xer_fprint(stdout, &asn_DEF_ItsPduHeader, header);
		}

		MAPEM_t *mapem = nullptr;
		ret = parse_mapem(atopo+s, sizeof(atopo)-s, &mapem);
		if (ret == 0)
		{
			dump_mapem(mapem);
			xer_fprint(stdout, &asn_DEF_MAPEM, mapem);
		}
	}
	//*/

	ms.add_msg({ aspat, sizeof(aspat) });

	if (!font.loadFromFile("FiraCode-Regular.ttf"))
	{
		return -1;
	}

	std::thread reader(reader_thread);

	window = new sf::RenderWindow(sf::VideoMode(1024, 786), "v2x");
	window->setVerticalSyncEnabled(true);
	while (window->isOpen())
	{
		sf::Event event = {};
		while (window->pollEvent(event))
		{
			// "close requested" event: we close the window
			if (event.type == sf::Event::Closed)
				window->close();
		}

		window->clear(sf::Color::Black);

		draw_data();

		window->display();
	}

	exit(0);

	return 0;
}
